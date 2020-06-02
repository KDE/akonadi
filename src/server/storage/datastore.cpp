/***************************************************************************
 *   Copyright (C) 2006 by Andreas Gungl <a.gungl@gmx.de>                  *
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "datastore.h"

#include "akonadi.h"
#include "collectionstatistics.h"
#include "dbconfig.h"
#include "dbinitializer.h"
#include "dbupdater.h"
#include "notificationmanager.h"
#include "tracer.h"
#include "transaction.h"
#include "selectquerybuilder.h"
#include "handlerhelper.h"
#include "countquerybuilder.h"
#include "parthelper.h"
#include "handler.h"
#include "collectionqueryhelper.h"
#include "akonadischema.h"
#include "parttypehelper.h"
#include "querycache.h"
#include "queryhelper.h"
#include "akonadiserver_debug.h"
#include "storagedebugger.h"
#include <utils.h>

#include <private/externalpartstorage_p.h>

#include <QCoreApplication>
#include <QString>
#include <QStringList>
#include <QThread>
#include <QThreadStorage>
#include <QTimer>
#include <QUuid>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QFile>
#include <QElapsedTimer>

#include <functional>

using namespace Akonadi;
using namespace Akonadi::Server;

bool DataStore::s_hasForeignKeyConstraints = false;
QMutex DataStore::sTransactionMutex = {};

static QThreadStorage<DataStore *> sInstances;

#define TRANSACTION_MUTEX_LOCK if ( DbType::isSystemSQLite( m_database ) ) sTransactionMutex.lock()
#define TRANSACTION_MUTEX_UNLOCK if ( DbType::isSystemSQLite( m_database ) ) sTransactionMutex.unlock()

#define setBoolPtr(ptr, val) \
    { \
        if ((ptr)) { \
            *(ptr) = (val); \
        } \
    }

std::unique_ptr<DataStoreFactory> DataStore::sFactory;

void DataStore::setFactory(std::unique_ptr<DataStoreFactory> factory)
{
    sFactory = std::move(factory);
}


/***************************************************************************
 *   DataStore                                                           *
 ***************************************************************************/
DataStore::DataStore(AkonadiServer &akonadi)
    : m_akonadi(akonadi)
    , m_dbOpened(false)
    , m_transactionLevel(0)
    , m_keepAliveTimer(nullptr)
{
    if (DbConfig::configuredDatabase()->driverName() == QLatin1String("QMYSQL")) {
        // Send a dummy query to MySQL every 1 hour to keep the connection alive,
        // otherwise MySQL just drops the connection and our subsequent queries fail
        // without properly reporting the error
        m_keepAliveTimer = new QTimer(this);
        m_keepAliveTimer->setInterval(3600 * 1000);
        QObject::connect(m_keepAliveTimer, &QTimer::timeout,
                         this, &DataStore::sendKeepAliveQuery);
    }
}

DataStore::~DataStore()
{
    if (m_dbOpened) {
        close();
    }
}

void DataStore::open()
{
    m_connectionName = QUuid::createUuid().toString() + QString::number(reinterpret_cast<qulonglong>(QThread::currentThread()));
    Q_ASSERT(!QSqlDatabase::contains(m_connectionName));

    m_database = QSqlDatabase::addDatabase(DbConfig::configuredDatabase()->driverName(), m_connectionName);
    DbConfig::configuredDatabase()->apply(m_database);

    if (!m_database.isValid()) {
        m_dbOpened = false;
        return;
    }
    m_dbOpened = m_database.open();

    if (!m_dbOpened) {
        debugLastDbError("Cannot open database.");
    } else {
        qCDebug(AKONADISERVER_LOG) << "Database" << m_database.databaseName() << "opened using driver" << m_database.driverName();
    }

    StorageDebugger::instance()->addConnection(reinterpret_cast<qint64>(this),
                                               QThread::currentThread()->objectName());
    connect(QThread::currentThread(), &QThread::objectNameChanged,
            this, [this](const QString &name) {
                if (!name.isEmpty()) {
                    StorageDebugger::instance()->changeConnection(reinterpret_cast<qint64>(this),
                                                                  name);
                }
            });

    DbConfig::configuredDatabase()->initSession(m_database);

    if (m_keepAliveTimer) {
        m_keepAliveTimer->start();
    }
}

QSqlDatabase DataStore::database()
{
    if (!m_dbOpened) {
        open();
    }
    return m_database;
}

void DataStore::close()
{

    if (m_keepAliveTimer) {
        m_keepAliveTimer->stop();
    }

    if (!m_dbOpened) {
        return;
    }

    if (inTransaction()) {
        // By setting m_transactionLevel to '1' here, we skip all nested transactions
        // and rollback the outermost transaction.
        m_transactionLevel = 1;
        rollbackTransaction();
    }

    QueryCache::clear();
    m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);

    StorageDebugger::instance()->removeConnection(reinterpret_cast<qint64>(this));

    m_dbOpened = false;
}

bool DataStore::init()
{
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    AkonadiSchema schema;
    DbInitializer::Ptr initializer = DbInitializer::createInstance(database(), &schema);
    if (!initializer->run()) {
        qCCritical(AKONADISERVER_LOG) << initializer->errorMsg();
        return false;
    }
    s_hasForeignKeyConstraints = initializer->hasForeignKeyConstraints();

    if (QFile::exists(QStringLiteral(":dbupdate.xml"))) {
        DbUpdater updater(database(), QStringLiteral(":dbupdate.xml"));
        if (!updater.run()) {
            return false;
        }
    } else {
        qCWarning(AKONADISERVER_LOG) << "Warning: dbupdate.xml not found, skipping updates";
    }

    if (!initializer->updateIndexesAndConstraints()) {
        qCCritical(AKONADISERVER_LOG) << initializer->errorMsg();
        return false;
    }

    // enable caching for some tables
    MimeType::enableCache(true);
    Flag::enableCache(true);
    Resource::enableCache(true);
    Collection::enableCache(true);
    PartType::enableCache(true);

    return true;
}

NotificationCollector *DataStore::notificationCollector()
{
    if (!mNotificationCollector) {
        mNotificationCollector = std::make_unique<NotificationCollector>(m_akonadi, this);
    }

    return mNotificationCollector.get();
}

DataStore *DataStore::self()
{
    if (!sInstances.hasLocalData()) {
        sInstances.setLocalData(sFactory->createStore());
    }
    return sInstances.localData();
}

bool DataStore::hasDataStore()
{
    return sInstances.hasLocalData();
}

/* --- ItemFlags ----------------------------------------------------- */

bool DataStore::setItemsFlags(const PimItem::List &items, const QVector<Flag> &flags,
                              bool *flagsChanged, const Collection &col_, bool silent)
{
    QSet<QString> removedFlags;
    QSet<QString> addedFlags;
    QVariantList insIds;
    QVariantList insFlags;
    Query::Condition delConds(Query::Or);
    Collection col = col_;

    setBoolPtr(flagsChanged, false);

    for (const PimItem &item : items) {
        const Flag::List itemFlags = item.flags();
        for (const Flag &flag : itemFlags) {
            if (!flags.contains(flag)) {
                removedFlags << flag.name();
                Query::Condition cond;
                cond.addValueCondition(PimItemFlagRelation::leftFullColumnName(), Query::Equals, item.id());
                cond.addValueCondition(PimItemFlagRelation::rightFullColumnName(), Query::Equals, flag.id());
                delConds.addCondition(cond);
            }
        }

        for (const Flag &flag : flags) {
            if (!itemFlags.contains(flag)) {
                addedFlags << flag.name();
                insIds << item.id();
                insFlags << flag.id();
            }
        }

        if (col.id() == -1) {
            col.setId(item.collectionId());
        } else if (col.id() != item.collectionId()) {
            col.setId(-2);
        }
    }

    if (!removedFlags.empty()) {
        QueryBuilder qb(PimItemFlagRelation::tableName(), QueryBuilder::Delete);
        qb.addCondition(delConds);
        if (!qb.exec()) {
            return false;
        }
    }

    if (!addedFlags.empty()) {
        QueryBuilder qb2(PimItemFlagRelation::tableName(), QueryBuilder::Insert);
        qb2.setColumnValue(PimItemFlagRelation::leftColumn(), insIds);
        qb2.setColumnValue(PimItemFlagRelation::rightColumn(), insFlags);
        qb2.setIdentificationColumn(QString());
        if (!qb2.exec()) {
            return false;
        }
    }

    if (!silent && (!addedFlags.isEmpty() || !removedFlags.isEmpty())) {
        QSet<QByteArray> addedFlagsBa;
        QSet<QByteArray> removedFlagsBa;
        for (const auto &addedFlag : qAsConst(addedFlags)) {
            addedFlagsBa.insert(addedFlag.toLatin1());
        }
        for (const auto &removedFlag : qAsConst(removedFlags)) {
            removedFlagsBa.insert(removedFlag.toLatin1());
        }
        notificationCollector()->itemsFlagsChanged(items, addedFlagsBa, removedFlagsBa, col);
    }

    setBoolPtr(flagsChanged, (addedFlags != removedFlags));

    return true;
}

bool DataStore::doAppendItemsFlag(const PimItem::List &items, const Flag &flag,
                                  const QSet<Entity::Id> &existing, const Collection &col_,
                                  bool silent)
{
    Collection col = col_;
    QVariantList flagIds;
    QVariantList appendIds;
    PimItem::List appendItems;
    for (const PimItem &item : items) {
        if (existing.contains(item.id())) {
            continue;
        }

        flagIds << flag.id();
        appendIds << item.id();
        appendItems << item;

        if (col.id() == -1) {
            col.setId(item.collectionId());
        } else if (col.id() != item.collectionId()) {
            col.setId(-2);
        }
    }

    if (appendItems.isEmpty()) {
        return true; // all items have the desired flags already
    }

    QueryBuilder qb2(PimItemFlagRelation::tableName(), QueryBuilder::Insert);
    qb2.setColumnValue(PimItemFlagRelation::leftColumn(), appendIds);
    qb2.setColumnValue(PimItemFlagRelation::rightColumn(), flagIds);
    qb2.setIdentificationColumn(QString());
    if (!qb2.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append flag" << flag.name() << "to Items" << appendIds;
        return false;
    }

    if (!silent) {
        notificationCollector()->itemsFlagsChanged(appendItems, {flag.name().toLatin1()}, {}, col);
    }

    return true;
}

bool DataStore::appendItemsFlags(const PimItem::List &items, const QVector<Flag> &flags,
                                 bool *flagsChanged, bool checkIfExists,
                                 const Collection &col, bool silent)
{
    QVariantList itemsIds;
    itemsIds.reserve(items.count());
    for (const PimItem &item : items) {
        itemsIds.append(item.id());
    }

    setBoolPtr(flagsChanged, false);

    for (const Flag &flag : flags) {
        QSet<PimItem::Id> existing;
        if (checkIfExists) {
            QueryBuilder qb(PimItemFlagRelation::tableName(), QueryBuilder::Select);
            Query::Condition cond;
            cond.addValueCondition(PimItemFlagRelation::rightColumn(), Query::Equals, flag.id());
            cond.addValueCondition(PimItemFlagRelation::leftColumn(), Query::In, itemsIds);
            qb.addColumn(PimItemFlagRelation::leftColumn());
            qb.addCondition(cond);

            if (!qb.exec()) {
                qCWarning(AKONADISERVER_LOG) << "Failed to retrieve existing flags for Items " << itemsIds;
                return false;
            }

            QSqlQuery query = qb.query();
            if (query.driver()->hasFeature(QSqlDriver::QuerySize)) {
                //The query size feature is not supported by the sqllite driver
                if (query.size() == items.count()) {
                    continue;
                }
                setBoolPtr(flagsChanged, true);
            }

            while (query.next()) {
                existing << query.value(0).value<PimItem::Id>();
            }
            if (!query.driver()->hasFeature(QSqlDriver::QuerySize)) {
                if (existing.size() != items.count()) {
                    setBoolPtr(flagsChanged, true);
                }
            }
            query.finish();
        }

        if (!doAppendItemsFlag(items, flag, existing, col, silent)) {
            return false;
        }
    }

    return true;
}

bool DataStore::removeItemsFlags(const PimItem::List &items, const QVector<Flag> &flags,
                                 bool *flagsChanged, const Collection &col_, bool silent)
{
    Collection col = col_;
    QSet<QString> removedFlags;
    QVariantList itemsIds;
    QVariantList flagsIds;

    setBoolPtr(flagsChanged, false);
    itemsIds.reserve(items.count());

    for (const PimItem &item : items) {
        itemsIds << item.id();
        if (col.id() == -1) {
            col.setId(item.collectionId());
        } else if (col.id() != item.collectionId()) {
            col.setId(-2);
        }
        for (int i = 0; i < flags.count(); ++i) {
            const QString flagName = flags[i].name();
            if (!removedFlags.contains(flagName)) {
                flagsIds << flags[i].id();
                removedFlags << flagName;
            }
        }
    }

    // Delete all given flags from all given items in one go
    QueryBuilder qb(PimItemFlagRelation::tableName(), QueryBuilder::Delete);
    Query::Condition cond(Query::And);
    cond.addValueCondition(PimItemFlagRelation::rightFullColumnName(), Query::In, flagsIds);
    cond.addValueCondition(PimItemFlagRelation::leftFullColumnName(), Query::In, itemsIds);
    qb.addCondition(cond);
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to remove flags" << flags << "from Items" << itemsIds;
        return false;
    }

    if (qb.query().numRowsAffected() != 0) {
        setBoolPtr(flagsChanged, true);
        if (!silent) {
            QSet<QByteArray> removedFlagsBa;
            for (const auto &remoteFlag : qAsConst(removedFlags)) {
                removedFlagsBa.insert(remoteFlag.toLatin1());
            }
            notificationCollector()->itemsFlagsChanged(items, {}, removedFlagsBa, col);
        }
    }

    return true;
}

/* --- ItemTags ----------------------------------------------------- */

bool DataStore::setItemsTags(const PimItem::List &items, const Tag::List &tags,
                             bool *tagsChanged, bool silent)
{
    QSet<qint64> removedTags;
    QSet<qint64> addedTags;
    QVariantList insIds;
    QVariantList insTags;
    Query::Condition delConds(Query::Or);

    setBoolPtr(tagsChanged, false);

    for (const PimItem &item : items) {
        const Tag::List itemTags = item.tags();
        for (const Tag &tag : itemTags) {
            if (!tags.contains(tag)) {
                // Remove tags from items that had it set
                removedTags << tag.id();
                Query::Condition cond;
                cond.addValueCondition(PimItemTagRelation::leftFullColumnName(), Query::Equals, item.id());
                cond.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::Equals, tag.id());
                delConds.addCondition(cond);
            }
        }

        for (const Tag &tag : tags) {
            if (!itemTags.contains(tag)) {
                // Add tags to items that did not have the tag
                addedTags << tag.id();
                insIds << item.id();
                insTags << tag.id();
            }
        }
    }

    if (!removedTags.empty()) {
        QueryBuilder qb(PimItemTagRelation::tableName(), QueryBuilder::Delete);
        qb.addCondition(delConds);
        if (!qb.exec()) {
            qCWarning(AKONADISERVER_LOG) << "Failed to remove tags" << removedTags << "from Items";
            return false;
        }
    }

    if (!addedTags.empty()) {
        QueryBuilder qb2(PimItemTagRelation::tableName(), QueryBuilder::Insert);
        qb2.setColumnValue(PimItemTagRelation::leftColumn(), insIds);
        qb2.setColumnValue(PimItemTagRelation::rightColumn(), insTags);
        qb2.setIdentificationColumn(QString());
        if (!qb2.exec()) {
            qCWarning(AKONADISERVER_LOG) << "Failed to add tags" << addedTags << "to Items";
            return false;
        }
    }

    if (!silent && (!addedTags.empty() || !removedTags.empty())) {
        notificationCollector()->itemsTagsChanged(items, addedTags, removedTags);
    }

    setBoolPtr(tagsChanged, (addedTags != removedTags));

    return true;
}

bool DataStore::doAppendItemsTag(const PimItem::List &items, const Tag &tag,
                                 const QSet<Entity::Id> &existing, const Collection &col,
                                 bool silent)
{
    QVariantList tagIds;
    QVariantList appendIds;
    PimItem::List appendItems;
    for (const PimItem &item : items) {
        if (existing.contains(item.id())) {
            continue;
        }

        tagIds << tag.id();
        appendIds << item.id();
        appendItems << item;
    }

    if (appendItems.isEmpty()) {
        return true; // all items have the desired tags already
    }

    QueryBuilder qb2(PimItemTagRelation::tableName(), QueryBuilder::Insert);
    qb2.setColumnValue(PimItemTagRelation::leftColumn(), appendIds);
    qb2.setColumnValue(PimItemTagRelation::rightColumn(), tagIds);
    qb2.setIdentificationColumn(QString());
    if (!qb2.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append tag" << tag << "to Items" << appendItems;
        return false;
    }

    if (!silent) {
        notificationCollector()->itemsTagsChanged(appendItems, {tag.id()}, {}, col);
    }

    return true;
}

bool DataStore::appendItemsTags(const PimItem::List &items, const Tag::List &tags,
                                bool *tagsChanged, bool checkIfExists,
                                const Collection &col, bool silent)
{
    QVariantList itemsIds;
    itemsIds.reserve(items.count());
    for (const PimItem &item : items) {
        itemsIds.append(item.id());
    }

    setBoolPtr(tagsChanged, false);

    for (const Tag &tag : tags) {
        QSet<PimItem::Id> existing;
        if (checkIfExists) {
            QueryBuilder qb(PimItemTagRelation::tableName(), QueryBuilder::Select);
            Query::Condition cond;
            cond.addValueCondition(PimItemTagRelation::rightColumn(), Query::Equals, tag.id());
            cond.addValueCondition(PimItemTagRelation::leftColumn(), Query::In, itemsIds);
            qb.addColumn(PimItemTagRelation::leftColumn());
            qb.addCondition(cond);

            if (!qb.exec()) {
                qCWarning(AKONADISERVER_LOG) << "Failed to retrieve existing tag" << tag << "for Items" << itemsIds;
                return false;
            }

            QSqlQuery query = qb.query();
            if (query.driver()->hasFeature(QSqlDriver::QuerySize)) {
                if (query.size() == items.count()) {
                    continue;
                }
                setBoolPtr(tagsChanged, true);
            }

            while (query.next()) {
                existing << query.value(0).value<PimItem::Id>();
            }
            if (!query.driver()->hasFeature(QSqlDriver::QuerySize)) {
                if (existing.size() != items.count()) {
                    setBoolPtr(tagsChanged, true);
                }
            }
            query.finish();
        }

        if (!doAppendItemsTag(items, tag, existing, col, silent)) {
            return false;
        }
    }

    return true;
}

bool DataStore::removeItemsTags(const PimItem::List &items, const Tag::List &tags,
                                bool *tagsChanged, bool silent)
{
    QSet<qint64> removedTags;
    QVariantList itemsIds;
    QVariantList tagsIds;

    setBoolPtr(tagsChanged, false);
    itemsIds.reserve(items.count());

    for (const PimItem &item : items) {
        itemsIds << item.id();
        for (int i = 0; i < tags.count(); ++i) {
            const qint64 tagId = tags[i].id();
            if (!removedTags.contains(tagId)) {
                tagsIds << tagId;
                removedTags << tagId;
            }
        }
    }

    // Delete all given tags from all given items in one go
    QueryBuilder qb(PimItemTagRelation::tableName(), QueryBuilder::Delete);
    Query::Condition cond(Query::And);
    cond.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::In, tagsIds);
    cond.addValueCondition(PimItemTagRelation::leftFullColumnName(), Query::In, itemsIds);
    qb.addCondition(cond);
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to remove tags" << tagsIds << "from Items" << itemsIds;
        return false;
    }

    if (qb.query().numRowsAffected() != 0) {
        setBoolPtr(tagsChanged, true);
        if (!silent) {
            notificationCollector()->itemsTagsChanged(items, QSet<qint64>(), removedTags);
        }
    }

    return true;
}

bool DataStore::removeTags(const Tag::List &tags, bool silent)
{
    // Currently the "silent" argument is only for API symmetry
    Q_UNUSED(silent);

    QVariantList removedTagsIds;
    QSet<qint64> removedTags;
    removedTagsIds.reserve(tags.count());
    removedTags.reserve(tags.count());
    for (const Tag &tag : tags) {
        removedTagsIds << tag.id();
        removedTags << tag.id();
    }

    // Get all PIM items that we will untag
    SelectQueryBuilder<PimItem> itemsQuery;
    itemsQuery.addJoin(QueryBuilder::LeftJoin, PimItemTagRelation::tableName(), PimItemTagRelation::leftFullColumnName(), PimItem::idFullColumnName());
    itemsQuery.addValueCondition(PimItemTagRelation::rightFullColumnName(), Query::In, removedTagsIds);

    if (!itemsQuery.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Removing tags failed: failed to query Items for given tags" << removedTagsIds;
        return false;
    }
    const PimItem::List items = itemsQuery.result();

    if (!items.isEmpty()) {
        notificationCollector()->itemsTagsChanged(items, QSet<qint64>(), removedTags);
    }

    for (const Tag &tag : tags) {
        // Emit special tagRemoved notification for each resource that owns the tag
        QueryBuilder qb(TagRemoteIdResourceRelation::tableName(), QueryBuilder::Select);
        qb.addColumn(TagRemoteIdResourceRelation::remoteIdFullColumnName());
        qb.addJoin(QueryBuilder::InnerJoin, Resource::tableName(),
                   TagRemoteIdResourceRelation::resourceIdFullColumnName(), Resource::idFullColumnName());
        qb.addColumn(Resource::nameFullColumnName());
        qb.addValueCondition(TagRemoteIdResourceRelation::tagIdFullColumnName(), Query::Equals, tag.id());
        if (!qb.exec()) {
            qCWarning(AKONADISERVER_LOG) << "Removing tags failed: failed to retrieve RIDs for tag" << tag.id();
            return false;
        }

        // Emit specialized notifications for each resource
        QSqlQuery query = qb.query();
        while (query.next()) {
            const QString rid = query.value(0).toString();
            const QByteArray resource = query.value(1).toByteArray();

            notificationCollector()->tagRemoved(tag, resource, rid);
        }
        query.finish();

        // And one for clients - without RID
        notificationCollector()->tagRemoved(tag, QByteArray(), QString());
    }

    // Just remove the tags, table constraints will take care of the rest
    QueryBuilder qb(Tag::tableName(), QueryBuilder::Delete);
    qb.addValueCondition(Tag::idColumn(), Query::In, removedTagsIds);
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to remove tags" << removedTagsIds;
        return false;
    }

    return true;
}

/* --- ItemParts ----------------------------------------------------- */

bool DataStore::removeItemParts(const PimItem &item, const QSet<QByteArray> &parts)
{
    SelectQueryBuilder<Part> qb;
    qb.addJoin(QueryBuilder::InnerJoin, PartType::tableName(), Part::partTypeIdFullColumnName(), PartType::idFullColumnName());
    qb.addValueCondition(Part::pimItemIdFullColumnName(), Query::Equals, item.id());
    qb.addCondition(PartTypeHelper::conditionFromFqNames(parts));

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Removing item parts failed: failed to query parts" << parts << "from Item " << item.id();
        return false;
    }

    const Part::List existingParts = qb.result();
    for (Part part : qAsConst(existingParts)) {  //krazy:exclude=foreach
        if (!PartHelper::remove(&part)) {
            qCWarning(AKONADISERVER_LOG) << "Failed to remove part" << part.id() << "(" << part.partType().ns()
                                         << ":" << part.partType().name() << ") from Item" << item.id();
            return false;
        }
    }

    notificationCollector()->itemChanged(item, parts);
    return true;
}

bool DataStore::invalidateItemCache(const PimItem &item)
{
    // find all payload item parts
    SelectQueryBuilder<Part> qb;
    qb.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(), PimItem::idFullColumnName(), Part::pimItemIdFullColumnName());
    qb.addJoin(QueryBuilder::InnerJoin, PartType::tableName(), Part::partTypeIdFullColumnName(), PartType::idFullColumnName());
    qb.addValueCondition(Part::pimItemIdFullColumnName(), Query::Equals, item.id());
    qb.addValueCondition(Part::dataFullColumnName(), Query::IsNot, QVariant());
    qb.addValueCondition(PartType::nsFullColumnName(), Query::Equals, QLatin1String("PLD"));
    qb.addValueCondition(PimItem::dirtyFullColumnName(), Query::Equals, false);

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to invalidate cache for Item" << item.id();
        return false;
    }

    const Part::List parts = qb.result();
    // clear data field
    for (Part part : parts) {
        if (!PartHelper::truncate(part)) {
            qCWarning(AKONADISERVER_LOG) << "Failed to truncate payload part" << part.id() << "(" << part.partType().ns()
                                         << ":" << part.partType().name() << ") of Item" << item.id();
            return false;
        }
    }

    return true;
}

/* --- Collection ------------------------------------------------------ */
bool DataStore::appendCollection(Collection &collection, const QStringList &mimeTypes, const QMap<QByteArray, QByteArray> &attributes)
{
    // no need to check for already existing collection with the same name,
    // a unique index on parent + name prevents that in the database
    if (!collection.insert()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append Collection" << collection.name() << "in resource"
                                     << collection.resource().name();
        return false;
    }

    if (!appendMimeTypeForCollection(collection.id(), mimeTypes)) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append mimetypes" << mimeTypes << "to new collection" << collection.name()
                                     << "(ID" << collection.id() << ") in resource" << collection.resource().name();
        return false;
    }

    for (auto it = attributes.cbegin(), end = attributes.cend(); it != end; ++it) {
        if (!addCollectionAttribute(collection, it.key(), it.value(), true)) {
            qCWarning(AKONADISERVER_LOG) << "Failed to append attribute" << it.key() << "to new collection" << collection.name()
                                         << "(ID" << collection.id() << ") in resource" << collection.resource().name();
            return false;
        }
    }

    notificationCollector()->collectionAdded(collection);
    return true;
}

bool DataStore::cleanupCollection(Collection &collection)
{
    if (!s_hasForeignKeyConstraints) {
        return cleanupCollection_slow(collection);
    }

    // db will do most of the work for us, we just deal with notifications and external payload parts here
    Q_ASSERT(s_hasForeignKeyConstraints);

    // collect item deletion notifications
    const PimItem::List items = collection.items();
    const QByteArray resource = collection.resource().name().toLatin1();

    // generate the notification before actually removing the data
    // TODO: we should try to get rid of this, requires client side changes to resources and Monitor though
    notificationCollector()->itemsRemoved(items, collection, resource);

    // remove all external payload parts
    QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
    qb.addColumn(Part::dataFullColumnName());
    qb.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(), Part::pimItemIdFullColumnName(), PimItem::idFullColumnName());
    qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
    qb.addValueCondition(Collection::idFullColumnName(), Query::Equals, collection.id());
    qb.addValueCondition(Part::storageFullColumnName(), Query::Equals, Part::External);
    qb.addValueCondition(Part::dataFullColumnName(), Query::IsNot, QVariant());
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to cleanup collection" << collection.name() << "(ID" << collection.id() << "):"
                                     << "Failed to query existing payload parts";
        return false;
    }

    try {
        while (qb.query().next()) {
            ExternalPartStorage::self()->removePartFile(
                ExternalPartStorage::resolveAbsolutePath(qb.query().value(0).toByteArray()));
        }
    } catch (const PartHelperException &e) {
        qb.query().finish();
        qCWarning(AKONADISERVER_LOG) << "PartHelperException while cleaning up collection" << collection.name()
                                     << "(ID" << collection.id() << "):" << e.what();
        return false;
    }
    qb.query().finish();

    // delete the collection itself, referential actions will do the rest
    notificationCollector()->collectionRemoved(collection);
    return collection.remove();
}

bool DataStore::cleanupCollection_slow(Collection &collection)
{
    Q_ASSERT(!s_hasForeignKeyConstraints);

    // delete the content
    const PimItem::List items = collection.items();
    const QByteArray resource = collection.resource().name().toLatin1();
    notificationCollector()->itemsRemoved(items, collection, resource);

    for (const PimItem &item : items) {
        if (!item.clearFlags()) {   // TODO: move out of loop and use only a single query
            qCWarning(AKONADISERVER_LOG) << "Slow cleanup of collection" << collection.name() << "(ID" << collection.id() <<")"
                                         << "failed: error clearing items flags";
            return false;
        }
        if (!PartHelper::remove(Part::pimItemIdColumn(), item.id())) {     // TODO: reduce to single query
            qCWarning(AKONADISERVER_LOG) << "Slow cleanup of collection" << collection.name() << "(ID" << collection.id() <<")"
                                         << "failed: error clearing item payload parts";

            return false;
        }

        if (!PimItem::remove(PimItem::idColumn(), item.id())) {     // TODO: move into single querya
            qCWarning(AKONADISERVER_LOG) << "Slow cleanup of collection" << collection.name() << "(ID" << collection.id() <<")"
                                         << "failed: error clearing items";
            return false;
        }

        if (!Entity::clearRelation<CollectionPimItemRelation>(item.id(), Entity::Right)) {     // TODO: move into single query
            qCWarning(AKONADISERVER_LOG) << "Slow cleanup of collection" << collection.name() << "(ID" << collection.id() <<")"
                                         << "failed: error clearing linked items";
            return false;
        }
    }

    // delete collection mimetypes
    collection.clearMimeTypes();
    Collection::clearPimItems(collection.id());

    // delete attributes
    Q_FOREACH (CollectionAttribute attr, collection.attributes()) { //krazy:exclude=foreach
        if (!attr.remove()) {
            qCWarning(AKONADISERVER_LOG) << "Slow cleanup of collection" << collection.name() << "(ID" << collection.id() << ")"
                                         << "failed: error clearing attribute" << attr.type();
            return false;
        }
    }

    // delete the collection itself
    notificationCollector()->collectionRemoved(collection);
    return collection.remove();
}

static bool recursiveSetResourceId(const Collection &collection, qint64 resourceId)
{
    Transaction transaction(DataStore::self(), QStringLiteral("RECURSIVE SET RESOURCEID"));

    QueryBuilder qb(Collection::tableName(), QueryBuilder::Update);
    qb.addValueCondition(Collection::parentIdColumn(), Query::Equals, collection.id());
    qb.setColumnValue(Collection::resourceIdColumn(), resourceId);
    qb.setColumnValue(Collection::remoteIdColumn(), QVariant());
    qb.setColumnValue(Collection::remoteRevisionColumn(), QVariant());
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to set resource ID" << resourceId << "to collection"
                                     << collection.name() << "(ID" << collection.id() << ")";
        return false;
    }

    // this is a cross-resource move, so also reset any resource-specific data (RID, RREV, etc)
    // as well as mark the items dirty to prevent cache purging before they have been written back
    qb = QueryBuilder(PimItem::tableName(), QueryBuilder::Update);
    qb.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, collection.id());
    qb.setColumnValue(PimItem::remoteIdColumn(), QVariant());
    qb.setColumnValue(PimItem::remoteRevisionColumn(), QVariant());
    const QDateTime now = QDateTime::currentDateTimeUtc();
    qb.setColumnValue(PimItem::datetimeColumn(), now);
    qb.setColumnValue(PimItem::atimeColumn(), now);
    qb.setColumnValue(PimItem::dirtyColumn(), true);
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed reset RID/RREV for PimItems in Collection" << collection.name()
                                     << "(ID" << collection.id() << ")";
        return false;
    }

    transaction.commit();

    Q_FOREACH (const Collection &col, collection.children()) {
        if (!recursiveSetResourceId(col, resourceId)) {
            return false;
        }
    }
    return true;
}

bool DataStore::moveCollection(Collection &collection, const Collection &newParent)
{
    if (collection.parentId() == newParent.id()) {
        return true;
    }

    if (!m_dbOpened) {
        return false;
    }

    if (!newParent.isValid()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to move collection" << collection.name() << "(ID"
                                     << collection.id() << "): invalid destination";
        return false;
    }

    const QByteArray oldResource = collection.resource().name().toLatin1();

    int resourceId = collection.resourceId();
    const Collection source = collection.parent();
    if (newParent.id() > 0) {   // not root
        resourceId = newParent.resourceId();
    }
    if (!CollectionQueryHelper::canBeMovedTo(collection, newParent)) {
        return false;
    }

    collection.setParentId(newParent.id());
    if (collection.resourceId() != resourceId) {
        collection.setResourceId(resourceId);
        collection.setRemoteId(QString());
        collection.setRemoteRevision(QString());
        if (!recursiveSetResourceId(collection, resourceId)) {
            return false;
        }
    }

    if (!collection.update()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to move Collection" << collection.name() << "(ID" << collection.id() << ")"
                                     << "into Collection" << collection.name() << "(ID" << collection.id() << ")";
        return false;
    }

    notificationCollector()->collectionMoved(collection, source, oldResource, newParent.resource().name().toLatin1());
    return true;
}

bool DataStore::appendMimeTypeForCollection(qint64 collectionId, const QStringList &mimeTypes)
{
    if (mimeTypes.isEmpty()) {
        return true;
    }

    for (const QString &mimeType : mimeTypes) {
        const auto &mt = MimeType::retrieveByNameOrCreate(mimeType);
        if (!mt.isValid()) {
            return false;
        }
        if (!Collection::addMimeType(collectionId, mt.id())) {
            qCWarning(AKONADISERVER_LOG) << "Failed to append mimetype" << mt.name() << "to Collection" << collectionId;
            return false;
        }
    }

    return true;
}

void DataStore::activeCachePolicy(Collection &col)
{
    if (!col.cachePolicyInherit()) {
        return;
    }

    Collection parent = col;
    while (parent.parentId() != 0) {
        parent = parent.parent();
        if (!parent.cachePolicyInherit()) {
            col.setCachePolicyCheckInterval(parent.cachePolicyCheckInterval());
            col.setCachePolicyCacheTimeout(parent.cachePolicyCacheTimeout());
            col.setCachePolicySyncOnDemand(parent.cachePolicySyncOnDemand());
            col.setCachePolicyLocalParts(parent.cachePolicyLocalParts());
            return;
        }
    }

    // ### system default
    col.setCachePolicyCheckInterval(-1);
    col.setCachePolicyCacheTimeout(-1);
    col.setCachePolicySyncOnDemand(false);
    col.setCachePolicyLocalParts(QStringLiteral("ALL"));
}

QVector<Collection> DataStore::virtualCollections(const PimItem &item)
{
    SelectQueryBuilder<Collection> qb;
    qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(),
               Collection::idFullColumnName(), CollectionPimItemRelation::leftFullColumnName());
    qb.addValueCondition(CollectionPimItemRelation::rightFullColumnName(), Query::Equals, item.id());

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to query virtual collections which PimItem" << item.id() << "belongs into";
        return QVector<Collection>();
    }

    return qb.result();
}

QMap<Entity::Id, QList<PimItem> > DataStore::virtualCollections(const PimItem::List &items)
{
    QueryBuilder qb(CollectionPimItemRelation::tableName(), QueryBuilder::Select);
    qb.addJoin(QueryBuilder::InnerJoin, Collection::tableName(),
               Collection::idFullColumnName(), CollectionPimItemRelation::leftFullColumnName());
    qb.addJoin(QueryBuilder::InnerJoin, PimItem::tableName(),
               PimItem::idFullColumnName(), CollectionPimItemRelation::rightFullColumnName());
    qb.addColumn(Collection::idFullColumnName());
    qb.addColumns(QStringList() << PimItem::idFullColumnName()
                  << PimItem::remoteIdFullColumnName()
                  << PimItem::remoteRevisionFullColumnName()
                  << PimItem::mimeTypeIdFullColumnName());
    qb.addSortColumn(Collection::idFullColumnName(), Query::Ascending);

    if (items.count() == 1) {
        qb.addValueCondition(CollectionPimItemRelation::rightFullColumnName(), Query::Equals, items.first().id());
    } else {
        QVariantList ids;
        ids.reserve(items.count());
        for (const PimItem &item : items) {
            ids << item.id();
        }
        qb.addValueCondition(CollectionPimItemRelation::rightFullColumnName(), Query::In, ids);
    }

    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to query virtual Collections which PimItems" << items << "belong into";
        return QMap<Entity::Id, QList<PimItem> >();
    }

    QSqlQuery query = qb.query();
    QMap<Entity::Id, QList<PimItem> > map;
    query.next();
    while (query.isValid()) {
        const qlonglong collectionId = query.value(0).toLongLong();
        QList<PimItem> &pimItems = map[collectionId];
        do {
            PimItem item;
            item.setId(query.value(1).toLongLong());
            item.setRemoteId(query.value(2).toString());
            item.setRemoteRevision(query.value(3).toString());
            item.setMimeTypeId(query.value(4).toLongLong());
            pimItems << item;
        } while (query.next() && query.value(0).toLongLong() == collectionId);
    }
    query.finish();

    return map;
}

/* --- PimItem ------------------------------------------------------- */
bool DataStore::appendPimItem(QVector<Part> &parts,
                              const QVector<Flag> &flags,
                              const MimeType &mimetype,
                              const Collection &collection,
                              const QDateTime &dateTime,
                              const QString &remote_id,
                              const QString &remoteRevision,
                              const QString &gid,
                              PimItem &pimItem)
{
    pimItem.setMimeTypeId(mimetype.id());
    pimItem.setCollectionId(collection.id());
    if (dateTime.isValid()) {
        pimItem.setDatetime(dateTime);
    }
    if (remote_id.isEmpty()) {
        // from application
        pimItem.setDirty(true);
    } else {
        // from resource
        pimItem.setRemoteId(remote_id);
        pimItem.setDirty(false);
    }
    pimItem.setRemoteRevision(remoteRevision);
    pimItem.setGid(gid);
    pimItem.setAtime(QDateTime::currentDateTimeUtc());

    if (!pimItem.insert()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append new PimItem into Collection" << collection.name()
                                     << "(ID" << collection.id() << ")";
        return false;
    }

    // insert every part
    if (!parts.isEmpty()) {
        //don't use foreach, the caller depends on knowing the part has changed, see the Append handler
        for (QVector<Part>::iterator it = parts.begin(); it != parts.end(); ++it) {

            (*it).setPimItemId(pimItem.id());
            if ((*it).datasize() < (*it).data().size()) {
                (*it).setDatasize((*it).data().size());
            }

//      qCDebug(AKONADISERVER_LOG) << "Insert from DataStore::appendPimItem";
            if (!PartHelper::insert(&(*it))) {
                qCWarning(AKONADISERVER_LOG) << "Failed to add part" << it->partType().name() << "to new PimItem" << pimItem.id();
                return false;
            }
        }
    }

    bool seen = false;
    for (const Flag &flag : flags) {
        seen |= (flag.name() == QLatin1String(AKONADI_FLAG_SEEN)
                 || flag.name() == QLatin1String(AKONADI_FLAG_IGNORED));
        if (!pimItem.addFlag(flag)) {
            qCWarning(AKONADISERVER_LOG) << "Failed to add flag" << flag.name() << "to new PimItem" << pimItem.id();
            return false;
        }
    }

//   qCDebug(AKONADISERVER_LOG) << "appendPimItem: " << pimItem;

    notificationCollector()->itemAdded(pimItem, seen, collection);
    return true;
}

bool DataStore::unhidePimItem(PimItem &pimItem)
{
    if (!m_dbOpened) {
        return false;
    }

    qCDebug(AKONADISERVER_LOG) << "DataStore::unhidePimItem(" << pimItem << ")";

    // FIXME: This is inefficient. Using a bit on the PimItemTable record would probably be some orders of magnitude faster...
    return removeItemParts(pimItem, { AKONADI_ATTRIBUTE_HIDDEN });
}

bool DataStore::unhideAllPimItems()
{
    if (!m_dbOpened) {
        return false;
    }

    qCDebug(AKONADISERVER_LOG) << "DataStore::unhideAllPimItems()";

    try {
        return PartHelper::remove(Part::partTypeIdFullColumnName(),
                                  PartTypeHelper::fromFqName(QStringLiteral("ATR"), QStringLiteral("HIDDEN")).id());
    } catch (...) {
    } // we can live with this failing

    return false;
}

bool DataStore::cleanupPimItems(const PimItem::List &items, bool silent)
{
    // generate relation removed notifications
    if (!silent) {
        for (const PimItem &item : items) {
            SelectQueryBuilder<Relation> relationQuery;
            relationQuery.addValueCondition(Relation::leftIdFullColumnName(), Query::Equals, item.id());
            relationQuery.addValueCondition(Relation::rightIdFullColumnName(), Query::Equals, item.id());
            relationQuery.setSubQueryMode(Query::Or);

            if (!relationQuery.exec()) {
                throw HandlerException("Failed to obtain relations");
            }
            const Relation::List relations = relationQuery.result();
            for (const Relation &relation : relations) {
                notificationCollector()->relationRemoved(relation);
            }
        }

        // generate the notification before actually removing the data
        notificationCollector()->itemsRemoved(items);
    }

    // FIXME: Create a single query to do this
    for (const auto &item : items) {
        if (!item.clearFlags()) {
            qCWarning(AKONADISERVER_LOG) << "Failed to clean up flags from PimItem" << item.id();
            return false;
        }
        if (!PartHelper::remove(Part::pimItemIdColumn(), item.id())) {
            qCWarning(AKONADISERVER_LOG) << "Failed to clean up parts from PimItem" << item.id();
            return false;
        }
        if (!PimItem::remove(PimItem::idColumn(), item.id())) {
            qCWarning(AKONADISERVER_LOG) << "Failed to remove PimItem" << item.id();
            return false;
        }

        if (!Entity::clearRelation<CollectionPimItemRelation>(item.id(), Entity::Right)) {
            qCWarning(AKONADISERVER_LOG) << "Failed to remove PimItem" << item.id() << "from linked collections";
            return false;
        }
    }

    return true;
}

bool DataStore::addCollectionAttribute(const Collection &col, const QByteArray &key, const QByteArray &value, bool silent)
{
    SelectQueryBuilder<CollectionAttribute> qb;
    qb.addValueCondition(CollectionAttribute::collectionIdColumn(), Query::Equals, col.id());
    qb.addValueCondition(CollectionAttribute::typeColumn(), Query::Equals, key);
    if (!qb.exec()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append attribute" << key << "to Collection" << col.name()
                                     << "(ID" << col.id() << "): Failed to query existing attribute";
        return false;
    }

    if (!qb.result().isEmpty()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append attribute" << key << "to Collection" << col.name()
                                     << "(ID" << col.id() << "): Attribute already exists";
        return false;
    }

    CollectionAttribute attr;
    attr.setCollectionId(col.id());
    attr.setType(key);
    attr.setValue(value);

    if (!attr.insert()) {
        qCWarning(AKONADISERVER_LOG) << "Failed to append attribute" << key << "to Collection" << col.name()
                                     << "(ID" << col.id() << ")";
        return false;
    }

    if (!silent) {
        notificationCollector()->collectionChanged(col, QList<QByteArray>() << key);
    }
    return true;
}

bool DataStore::removeCollectionAttribute(const Collection &col, const QByteArray &key)
{
    SelectQueryBuilder<CollectionAttribute> qb;
    qb.addValueCondition(CollectionAttribute::collectionIdColumn(), Query::Equals, col.id());
    qb.addValueCondition(CollectionAttribute::typeColumn(), Query::Equals, key);
    if (!qb.exec()) {
        throw HandlerException("Unable to query for collection attribute");
    }

    const QVector<CollectionAttribute> result = qb.result();
    for (CollectionAttribute attr : result) {
        if (!attr.remove()) {
            throw HandlerException("Unable to remove collection attribute");
        }
    }

    if (!result.isEmpty()) {
        notificationCollector()->collectionChanged(col, QList<QByteArray>() << key);
        return true;
    }
    return false;
}

void DataStore::debugLastDbError(const char *actionDescription) const
{
    qCCritical(AKONADISERVER_LOG) << "Database error:" << actionDescription;
    qCCritical(AKONADISERVER_LOG) << "  Last driver error:" << m_database.lastError().driverText();
    qCCritical(AKONADISERVER_LOG) << "  Last database error:" << m_database.lastError().databaseText();

    m_akonadi.tracer().error("DataStore (Database Error)",
                          QStringLiteral("%1\nDriver said: %2\nDatabase said:%3")
                          .arg(QString::fromLatin1(actionDescription),
                               m_database.lastError().driverText(),
                               m_database.lastError().databaseText()));
}

void DataStore::debugLastQueryError(const QSqlQuery &query, const char *actionDescription) const
{
    qCCritical(AKONADISERVER_LOG) << "Query error:" << actionDescription;
    qCCritical(AKONADISERVER_LOG) << "  Last error message:" << query.lastError().text();
    qCCritical(AKONADISERVER_LOG) << "  Last driver error:" << m_database.lastError().driverText();
    qCCritical(AKONADISERVER_LOG) << "  Last database error:" << m_database.lastError().databaseText();

    m_akonadi.tracer().error("DataStore (Database Query Error)",
                          QStringLiteral("%1: %2")
                          .arg(QString::fromLatin1(actionDescription),
                               query.lastError().text()));
}

// static
QString DataStore::dateTimeFromQDateTime(const QDateTime &dateTime)
{
    QDateTime utcDateTime = dateTime;
    if (utcDateTime.timeSpec() != Qt::UTC) {
        utcDateTime.toUTC();
    }
    return utcDateTime.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
}

// static
QDateTime DataStore::dateTimeToQDateTime(const QByteArray &dateTime)
{
    return QDateTime::fromString(QString::fromLatin1(dateTime), QStringLiteral("yyyy-MM-dd hh:mm:ss"));
}

bool DataStore::doRollback()
{
    QSqlDriver *driver = m_database.driver();
    QElapsedTimer timer; timer.start();
    driver->rollbackTransaction();
    StorageDebugger::instance()->removeTransaction(reinterpret_cast<qint64>(this),
                                                   false, timer.elapsed(),
                                                   m_database.lastError().text());
    if (m_database.lastError().isValid()) {
        TRANSACTION_MUTEX_UNLOCK;
        debugLastDbError("DataStore::rollbackTransaction");
        return false;
    }
    TRANSACTION_MUTEX_UNLOCK;
    return true;
}

void DataStore::transactionKilledByDB()
{
    m_transactionKilledByDB = true;
    cleanupAfterRollback();
    Q_EMIT transactionRolledBack();
}

bool DataStore::beginTransaction(const QString &name)
{
    if (!m_dbOpened) {
        return false;
    }

    if (m_transactionLevel == 0 || m_transactionKilledByDB) {
        m_transactionKilledByDB = false;
        QElapsedTimer timer;
        timer.start();
        TRANSACTION_MUTEX_LOCK;
        if (DbType::type(m_database) == DbType::Sqlite) {
            m_database.exec(QStringLiteral("BEGIN IMMEDIATE TRANSACTION"));
            StorageDebugger::instance()->addTransaction(reinterpret_cast<qint64>(this),
                                                        name, timer.elapsed(),
                                                        m_database.lastError().text());
            if (m_database.lastError().isValid()) {
                debugLastDbError("DataStore::beginTransaction (SQLITE)");
                TRANSACTION_MUTEX_UNLOCK;
                return false;
            }
        } else {
            m_database.driver()->beginTransaction();
            StorageDebugger::instance()->addTransaction(reinterpret_cast<qint64>(this),
                                                        name, timer.elapsed(),
                                                        m_database.lastError().text());
            if (m_database.lastError().isValid()) {
                debugLastDbError("DataStore::beginTransaction");
                TRANSACTION_MUTEX_UNLOCK;
                return false;
            }
        }

        if (DbType::type(m_database) == DbType::PostgreSQL) {
            // Make constraints check deferred in PostgreSQL. Allows for
            // INSERT INTO mimetypetable (name) VALUES ('foo') RETURNING id;
            // INSERT INTO collectionmimetyperelation (collection_id, mimetype_id) VALUES (x, y)
            // where "y" refers to the newly inserted mimetype
            m_database.exec(QStringLiteral("SET CONSTRAINTS ALL DEFERRED"));
        }
    }

    ++m_transactionLevel;

    return true;
}

bool DataStore::rollbackTransaction()
{
    if (!m_dbOpened) {
        return false;
    }

    if (m_transactionLevel == 0) {
        qCWarning(AKONADISERVER_LOG) << "DataStore::rollbackTransaction(): No transaction in progress!";
        return false;
    }

    --m_transactionLevel;

    if (m_transactionLevel == 0 && !m_transactionKilledByDB) {
        doRollback();
        cleanupAfterRollback();
        Q_EMIT transactionRolledBack();
    }

    return true;
}

bool DataStore::commitTransaction()
{
    if (!m_dbOpened) {
        return false;
    }

    if (m_transactionLevel == 0) {
        qCWarning(AKONADISERVER_LOG) << "DataStore::commitTransaction(): No transaction in progress!";
        return false;
    }

    if (m_transactionLevel == 1) {
        if (m_transactionKilledByDB) {
            qCWarning(AKONADISERVER_LOG) << "DataStore::commitTransaction(): Cannot commit, transaction was killed by mysql deadlock handling!";
            return false;
        }
        QSqlDriver *driver = m_database.driver();
        QElapsedTimer timer;
        timer.start();
        driver->commitTransaction();
        StorageDebugger::instance()->removeTransaction(reinterpret_cast<qint64>(this),
                                                       true, timer.elapsed(),
                                                       m_database.lastError().text());
        if (m_database.lastError().isValid()) {
            debugLastDbError("DataStore::commitTransaction");
            rollbackTransaction();
            return false;
        } else {
            TRANSACTION_MUTEX_UNLOCK;
            m_transactionLevel--;
            Q_EMIT transactionCommitted();
        }
    } else {
        m_transactionLevel--;
    }
    return true;
}

bool DataStore::inTransaction() const
{
    return m_transactionLevel > 0;
}

void DataStore::sendKeepAliveQuery()
{
    if (m_database.isOpen()) {
        QSqlQuery query(m_database);
        query.exec(QStringLiteral("SELECT 1"));
    }
}

void DataStore::cleanupAfterRollback()
{
    MimeType::invalidateCompleteCache();
    Flag::invalidateCompleteCache();
    Resource::invalidateCompleteCache();
    Collection::invalidateCompleteCache();
    PartType::invalidateCompleteCache();
    m_akonadi.collectionStatistics().expireCache();
    QueryCache::clear();
}
