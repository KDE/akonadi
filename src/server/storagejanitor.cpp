/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "storagejanitor.h"

#include "storage/queryhelper.h"
#include "storage/transaction.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/parthelper.h"
#include "storage/dbconfig.h"
#include "storage/collectionstatistics.h"
#include "search/searchrequest.h"
#include "search/searchmanager.h"
#include "resourcemanager.h"
#include "entities.h"
#include "dbusconnectionpool.h"
#include "agentmanagerinterface.h"
#include "akonadiserver_debug.h"

#include <private/dbus_p.h>
#include <private/imapset_p.h>
#include <private/protocol_p.h>
#include <private/standarddirs_p.h>
#include <private/externalpartstorage_p.h>

#include <QStringBuilder>
#include <QDBusConnection>
#include <QSqlQuery>
#include <QSqlError>
#include <QDir>
#include <qdiriterator.h>
#include <QDateTime>

#include <algorithm>

using namespace Akonadi;
using namespace Akonadi::Server;

StorageJanitor::StorageJanitor(QObject *parent)
    : AkThread(QStringLiteral("StorageJanitor"), QThread::IdlePriority, parent)
    , m_lostFoundCollectionId(-1)
{
}

StorageJanitor::~StorageJanitor()
{
    quitThread();
}

void StorageJanitor::init()
{
    AkThread::init();

    QDBusConnection conn = DBusConnectionPool::threadConnection();
    conn.registerService(DBus::serviceName(DBus::StorageJanitor));
    conn.registerObject(QStringLiteral(AKONADI_DBUS_STORAGEJANITOR_PATH), this,
                        QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);
}

void StorageJanitor::quit()
{
    QDBusConnection conn = DBusConnectionPool::threadConnection();
    conn.unregisterObject(QStringLiteral(AKONADI_DBUS_STORAGEJANITOR_PATH), QDBusConnection::UnregisterTree);
    conn.unregisterService(DBus::serviceName(DBus::StorageJanitor));
    conn.disconnectFromBus(conn.name());

    // Make sure all childrens are deleted within context of this thread
    qDeleteAll(children());

    qCDebug(AKONADISERVER_LOG) << "chainup()";
    AkThread::quit();
}

void StorageJanitor::check() // implementation of `akonadictl fsck`
{
    m_lostFoundCollectionId = -1; // start with a fresh one each time

    inform("Looking for resources in the DB not matching a configured resource...");
    findOrphanedResources();

    inform("Looking for collections not belonging to a valid resource...");
    findOrphanedCollections();

    inform("Checking collection tree consistency...");
    const Collection::List cols = Collection::retrieveAll();
    std::for_each(cols.begin(), cols.end(), [this](const Collection & col) {
        checkPathToRoot(col);
    });

    inform("Looking for items not belonging to a valid collection...");
    findOrphanedItems();

    inform("Looking for item parts not belonging to a valid item...");
    findOrphanedParts();

    inform("Looking for item flags not belonging to a valid item...");
    findOrphanedPimItemFlags();

    inform("Looking for overlapping external parts...");
    findOverlappingParts();

    inform("Verifying external parts...");
    verifyExternalParts();

    inform("Checking size treshold changes...");
    checkSizeTreshold();

    inform("Looking for dirty objects...");
    findDirtyObjects();

    inform("Looking for rid-duplicates not matching the content mime-type of the parent collection");
    findRIDDuplicates();

    inform("Migrating parts to new cache hierarchy...");
    migrateToLevelledCacheHierarchy();

    inform("Checking search index consistency...");
    findOrphanSearchIndexEntries();

    inform("Flushing collection statistics memory cache...");
    CollectionStatistics::self()->expireCache();

    /* TODO some ideas for further checks:
     * the collection tree is non-cyclic
     * content type constraints of collections are not violated
     * find unused flags
     * find unused mimetypes
     * check for dead entries in relation tables
     * check if part size matches file size
     */

    inform("Consistency check done.");

    Q_EMIT done();
}

qint64 StorageJanitor::lostAndFoundCollection()
{
    if (m_lostFoundCollectionId > 0) {
        return m_lostFoundCollectionId;
    }

    Transaction transaction(DataStore::self(), QStringLiteral("JANITOR LOST+FOUND"));
    Resource lfRes = Resource::retrieveByName(QStringLiteral("akonadi_lost+found_resource"));
    if (!lfRes.isValid()) {
        lfRes.setName(QStringLiteral("akonadi_lost+found_resource"));
        if (!lfRes.insert()) {
            qCCritical(AKONADISERVER_LOG) << "Failed to create lost+found resource!";
        }
    }

    Collection lfRoot;
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, lfRes.id());
    qb.addValueCondition(Collection::parentIdFullColumnName(), Query::Is, QVariant());
    if (!qb.exec()) {
        qCCritical(AKONADISERVER_LOG) << "Failed to query top level collections";
        return -1;
    }
    const Collection::List cols = qb.result();
    if (cols.size() > 1) {
        qCCritical(AKONADISERVER_LOG) << "More than one top-level lost+found collection!?";
    } else if (cols.size() == 1) {
        lfRoot = cols.first();
    } else {
        lfRoot.setName(QStringLiteral("lost+found"));
        lfRoot.setResourceId(lfRes.id());
        lfRoot.setCachePolicyLocalParts(QStringLiteral("ALL"));
        lfRoot.setCachePolicyCacheTimeout(-1);
        lfRoot.setCachePolicyInherit(false);
        if (!lfRoot.insert()) {
            qCCritical(AKONADISERVER_LOG) << "Failed to create lost+found root.";
        }
        DataStore::self()->notificationCollector()->collectionAdded(lfRoot, lfRes.name().toUtf8());
    }

    Collection lfCol;
    lfCol.setName(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    lfCol.setResourceId(lfRes.id());
    lfCol.setParentId(lfRoot.id());
    if (!lfCol.insert()) {
        qCCritical(AKONADISERVER_LOG) << "Failed to create lost+found collection!";
    }

    Q_FOREACH (const MimeType &mt, MimeType::retrieveAll()) {
        lfCol.addMimeType(mt);
    }

    DataStore::self()->notificationCollector()->collectionAdded(lfCol, lfRes.name().toUtf8());

    transaction.commit();
    m_lostFoundCollectionId = lfCol.id();
    return m_lostFoundCollectionId;
}

void StorageJanitor::findOrphanedResources()
{
    SelectQueryBuilder<Resource> qbres;
    OrgFreedesktopAkonadiAgentManagerInterface iface(
        DBus::serviceName(DBus::Control),
        QStringLiteral("/AgentManager"),
        QDBusConnection::sessionBus(),
        this);
    if (!iface.isValid()) {
        inform(QStringLiteral("ERROR: Couldn't talk to %1").arg(DBus::Control));
        return;
    }
    const QStringList knownResources = iface.agentInstances();
    if (knownResources.isEmpty()) {
        inform(QStringLiteral("ERROR: no known resources. This must be a mistake?"));
        return;
    }
    qCDebug(AKONADISERVER_LOG) << "Known resources:" << knownResources;
    qbres.addValueCondition(Resource::nameFullColumnName(), Query::NotIn, QVariant(knownResources));
    qbres.addValueCondition(Resource::idFullColumnName(), Query::NotEquals, 1);   // skip akonadi_search_resource
    if (!qbres.exec()) {
        inform("Failed to query known resources, skipping test");
        return;
    }
    //qCDebug(AKONADISERVER_LOG) << "SQL:" << qbres.query().lastQuery();
    const Resource::List orphanResources = qbres.result();
    const int orphanResourcesSize(orphanResources.size());
    if (orphanResourcesSize > 0) {
        QStringList resourceNames;
        resourceNames.reserve(orphanResourcesSize);
        for (const Resource &resource : orphanResources) {
            resourceNames.append(resource.name());
        }
        inform(QStringLiteral("Found %1 orphan resources: %2").arg(orphanResourcesSize). arg(resourceNames.join(QLatin1Char(','))));
        for (const QString &resourceName : qAsConst(resourceNames)) {
            inform(QStringLiteral("Removing resource %1").arg(resourceName));
            ResourceManager::self()->removeResourceInstance(resourceName);
        }
    }
}

void StorageJanitor::findOrphanedCollections()
{
    SelectQueryBuilder<Collection> qb;
    qb.addJoin(QueryBuilder::LeftJoin, Resource::tableName(), Collection::resourceIdFullColumnName(), Resource::idFullColumnName());
    qb.addValueCondition(Resource::idFullColumnName(), Query::Is, QVariant());

    if (!qb.exec()) {
        inform("Failed to query orphaned collections, skipping test");
        return;
    }
    const Collection::List orphans = qb.result();
    if (!orphans.isEmpty()) {
        inform(QLatin1Literal("Found ") + QString::number(orphans.size()) + QLatin1Literal(" orphan collections."));
        // TODO: attach to lost+found resource
    }
}

void StorageJanitor::checkPathToRoot(const Collection &col)
{
    if (col.parentId() == 0) {
        return;
    }
    const Collection parent = col.parent();
    if (!parent.isValid()) {
        inform(QLatin1Literal("Collection \"") + col.name() + QLatin1Literal("\" (id: ") + QString::number(col.id())
               + QLatin1Literal(") has no valid parent."));
        // TODO fix that by attaching to a top-level lost+found folder
        return;
    }

    if (col.resourceId() != parent.resourceId()) {
        inform(QLatin1Literal("Collection \"") + col.name() + QLatin1Literal("\" (id: ") + QString::number(col.id())
               + QLatin1Literal(") belongs to a different resource than its parent."));
        // can/should we actually fix that?
    }

    checkPathToRoot(parent);
}

void StorageJanitor::findOrphanedItems()
{
    SelectQueryBuilder<PimItem> qb;
    qb.addJoin(QueryBuilder::LeftJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
    qb.addValueCondition(Collection::idFullColumnName(), Query::Is, QVariant());
    if (!qb.exec()) {
        inform("Failed to query orphaned items, skipping test");
        return;
    }
    const PimItem::List orphans = qb.result();
    if (!orphans.isEmpty()) {
        inform(QLatin1Literal("Found ") + QString::number(orphans.size()) + QLatin1Literal(" orphan items."));
        // Attach to lost+found collection
        Transaction transaction(DataStore::self(), QStringLiteral("JANITOR ORPHANS"));
        QueryBuilder qb(PimItem::tableName(), QueryBuilder::Update);
        qint64 col = lostAndFoundCollection();
        if (col == -1) {
            return;
        }
        qb.setColumnValue(PimItem::collectionIdFullColumnName(), col);
        QVector<ImapSet::Id> imapIds;
        imapIds.reserve(orphans.count());
        for (const PimItem &item : qAsConst(orphans)) {
            imapIds.append(item.id());
        }
        ImapSet set;
        set.add(imapIds);
        QueryHelper::setToQuery(set, PimItem::idFullColumnName(), qb);
        if (qb.exec() && transaction.commit()) {
            inform(QLatin1Literal("Moved orphan items to collection ") + QString::number(col));
        } else {
            inform(QLatin1Literal("Error moving orphan items to collection ") + QString::number(col) + QLatin1Literal(" : ") + qb.query().lastError().text());
        }
    }
}

void StorageJanitor::findOrphanedParts()
{
    SelectQueryBuilder<Part> qb;
    qb.addJoin(QueryBuilder::LeftJoin, PimItem::tableName(), Part::pimItemIdFullColumnName(), PimItem::idFullColumnName());
    qb.addValueCondition(PimItem::idFullColumnName(), Query::Is, QVariant());
    if (!qb.exec()) {
        inform("Failed to query orphaned parts, skipping test");
        return;
    }
    const Part::List orphans = qb.result();
    if (!orphans.isEmpty()) {
        inform(QLatin1Literal("Found ") + QString::number(orphans.size()) + QLatin1Literal(" orphan parts."));
        // TODO: create lost+found items for those? delete?
    }
}

void StorageJanitor:: findOrphanedPimItemFlags()
{
    QueryBuilder sqb(PimItemFlagRelation::tableName(), QueryBuilder::Select);
    sqb.addColumn(PimItemFlagRelation::leftFullColumnName());
    sqb.addJoin(QueryBuilder::LeftJoin, PimItem::tableName(), PimItemFlagRelation::leftFullColumnName(), PimItem::idFullColumnName());
    sqb.addValueCondition(PimItem::idFullColumnName(), Query::Is, QVariant());
    if (!sqb.exec()) {
        inform("Failed to query orphaned item flags, skipping test");
        return;
    }
    QVector<ImapSet::Id> imapIds;
    int count = 0;
    while (sqb.query().next()) {
        ++count;
        imapIds.append(sqb.query().value(0).toInt());
    }

    if (count > 0) {
        ImapSet set;
        set.add(imapIds);
        QueryBuilder qb(PimItemFlagRelation::tableName(), QueryBuilder::Delete);
        QueryHelper::setToQuery(set, PimItemFlagRelation::leftFullColumnName(), qb);
        if (!qb.exec()) {
            qCCritical(AKONADISERVER_LOG) << "Error:" << qb.query().lastError().text();
            return;
        }

        inform(QLatin1Literal("Found and deleted ") + QString::number(count) + QLatin1Literal(" orphan pim item flags."));
    }
}

void StorageJanitor::findOverlappingParts()
{
    QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
    qb.addColumn(Part::dataColumn());
    qb.addColumn(QLatin1Literal("count(") + Part::idColumn() + QLatin1Literal(") as cnt"));
    qb.addValueCondition(Part::storageColumn(), Query::Equals, Part::External);
    qb.addValueCondition(Part::dataColumn(), Query::IsNot, QVariant());
    qb.addGroupColumn(Part::dataColumn());
    qb.addValueCondition(QLatin1Literal("count(") + Part::idColumn() + QLatin1Literal(")"), Query::Greater, 1, QueryBuilder::HavingCondition);
    if (!qb.exec()) {
        inform("Failed to query overlapping parts, skipping test");
        return;
    }

    int count = 0;
    while (qb.query().next()) {
        ++count;
        inform(QLatin1Literal("Found overlapping part data: ") + qb.query().value(0).toString());
        // TODO: uh oh, this is bad, how do we recover from that?
    }

    if (count > 0) {
        inform(QLatin1Literal("Found ") + QString::number(count) + QLatin1Literal(" overlapping parts - bad."));
    }
}

void StorageJanitor::verifyExternalParts()
{
    QSet<QString> existingFiles;
    QSet<QString> usedFiles;

    // list all files
    const QString dataDir = StandardDirs::saveDir("data", QStringLiteral("file_db_data"));
    QDirIterator it(dataDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        existingFiles.insert(it.next());
    }
    existingFiles.remove(dataDir + QDir::separator() + QLatin1Char('.'));
    existingFiles.remove(dataDir + QDir::separator() + QLatin1String(".."));
    inform(QLatin1Literal("Found ") + QString::number(existingFiles.size()) + QLatin1Literal(" external files."));

    // list all parts from the db which claim to have an associated file
    QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
    qb.addColumn(Part::dataColumn());
    qb.addColumn(Part::pimItemIdColumn());
    qb.addColumn(Part::idColumn());
    qb.addValueCondition(Part::storageColumn(), Query::Equals, Part::External);
    qb.addValueCondition(Part::dataColumn(), Query::IsNot, QVariant());
    if (!qb.exec()) {
        inform("Failed to query existing parts, skipping test");
        return;
    }
    while (qb.query().next()) {
        QString partPath = ExternalPartStorage::resolveAbsolutePath(qb.query().value(0).toByteArray());
        const Entity::Id pimItemId = qb.query().value(1).value<Entity::Id>();
        const Entity::Id id = qb.query().value(2).value<Entity::Id>();
        if (existingFiles.contains(partPath)) {
            usedFiles.insert(partPath);
        } else {
            inform(QLatin1Literal("Cleaning up missing external file: ") + partPath + QLatin1Literal(" for item: ") + QString::number(pimItemId) + QLatin1Literal(" on part: ") + QString::number(id));

            Part part;
            part.setId(id);
            part.setPimItemId(pimItemId);
            part.setData(QByteArray());
            part.setDatasize(0);
            part.setStorage(Part::Internal);
            part.update();
        }
    }
    inform(QLatin1Literal("Found ") + QString::number(usedFiles.size()) + QLatin1Literal(" external parts."));

    // see what's left and move it to lost+found
    const QSet<QString> unreferencedFiles = existingFiles - usedFiles;
    if (!unreferencedFiles.isEmpty()) {
        const QString lfDir = StandardDirs::saveDir("data", QStringLiteral("file_lost+found"));
        for (const QString &file : unreferencedFiles) {
            inform(QLatin1Literal("Found unreferenced external file: ") + file);
            const QFileInfo f(file);
            QFile::rename(file, lfDir + QDir::separator() + f.fileName());
        }
        inform(QStringLiteral("Moved %1 unreferenced files to lost+found.").arg(unreferencedFiles.size()));
    } else {
        inform("Found no unreferenced external files.");
    }
}

void StorageJanitor::findDirtyObjects()
{
    SelectQueryBuilder<Collection> cqb;
    cqb.setSubQueryMode(Query::Or);
    cqb.addValueCondition(Collection::remoteIdColumn(), Query::Is, QVariant());
    cqb.addValueCondition(Collection::remoteIdColumn(), Query::Equals, QString());
    if (!cqb.exec()) {
        inform("Failed to query collections without RID, skipping test");
        return;
    }
    const Collection::List ridLessCols = cqb.result();
    for (const Collection &col : ridLessCols) {
        inform(QLatin1Literal("Collection \"") + col.name() + QLatin1Literal("\" (id: ") + QString::number(col.id())
               + QLatin1Literal(") has no RID."));
    }
    inform(QLatin1Literal("Found ") + QString::number(ridLessCols.size()) + QLatin1Literal(" collections without RID."));

    SelectQueryBuilder<PimItem> iqb1;
    iqb1.setSubQueryMode(Query::Or);
    iqb1.addValueCondition(PimItem::remoteIdColumn(), Query::Is, QVariant());
    iqb1.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, QString());
    if (!iqb1.exec()) {
        inform("Failed to query items without RID, skipping test");
        return;
    }
    const PimItem::List ridLessItems = iqb1.result();
    for (const PimItem &item : ridLessItems) {
        inform(QLatin1Literal("Item \"") + QString::number(item.id()) + QLatin1Literal("\" has no RID."));
    }
    inform(QLatin1Literal("Found ") + QString::number(ridLessItems.size()) + QLatin1Literal(" items without RID."));

    SelectQueryBuilder<PimItem> iqb2;
    iqb2.addValueCondition(PimItem::dirtyColumn(), Query::Equals, true);
    iqb2.addValueCondition(PimItem::remoteIdColumn(), Query::IsNot, QVariant());
    iqb2.addSortColumn(PimItem::idFullColumnName());
    if (!iqb2.exec()) {
        inform("Failed to query dirty items, skipping test");
        return;
    }
    const PimItem::List dirtyItems = iqb2.result();
    for (const PimItem &item : dirtyItems) {
        inform(QLatin1Literal("Item \"") + QString::number(item.id()) + QLatin1Literal("\" has RID and is dirty."));
    }
    inform(QLatin1Literal("Found ") + QString::number(dirtyItems.size()) + QLatin1Literal(" dirty items."));
}

void StorageJanitor::findRIDDuplicates()
{
    QueryBuilder qb(Collection::tableName(), QueryBuilder::Select);
    qb.addColumn(Collection::idColumn());
    qb.addColumn(Collection::nameColumn());
    qb.exec();

    while (qb.query().next()) {
        const Collection::Id colId = qb.query().value(0).value<Collection::Id>();
        const QString name = qb.query().value(1).toString();
        inform(QStringLiteral("Checking ") + name);

        QueryBuilder duplicates(PimItem::tableName(), QueryBuilder::Select);
        duplicates.addColumn(PimItem::remoteIdColumn());
        duplicates.addColumn(QStringLiteral("count(") + PimItem::idColumn() + QStringLiteral(") as cnt"));
        duplicates.addValueCondition(PimItem::remoteIdColumn(), Query::IsNot, QVariant());
        duplicates.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, colId);
        duplicates.addGroupColumn(PimItem::remoteIdColumn());
        duplicates.addValueCondition(QStringLiteral("count(") + PimItem::idColumn() + QLatin1Char(')'), Query::Greater, 1, QueryBuilder::HavingCondition);
        duplicates.exec();

        Akonadi::Server::Collection col = Akonadi::Server::Collection::retrieveById(colId);
        const QVector<Akonadi::Server::MimeType> contentMimeTypes = col.mimeTypes();
        QVariantList contentMimeTypesVariantList;
        contentMimeTypesVariantList.reserve(contentMimeTypes.count());
        for (const Akonadi::Server::MimeType &mimeType : contentMimeTypes) {
            contentMimeTypesVariantList << mimeType.id();
        }
        while (duplicates.query().next()) {
            const QString rid = duplicates.query().value(0).toString();
            inform(QStringLiteral("Found duplicates ") + rid);

            Query::Condition condition(Query::And);
            condition.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, rid);
            condition.addValueCondition(PimItem::mimeTypeIdColumn(), Query::NotIn, contentMimeTypesVariantList);
            condition.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, colId);

            QueryBuilder items(PimItem::tableName(), QueryBuilder::Select);
            items.addColumn(PimItem::idColumn());
            items.addCondition(condition);
            if (!items.exec()) {
                inform(QStringLiteral("Error while deleting duplicates: ") + items.query().lastError().text());
                continue;
            }
            QVariantList itemsIds;
            while (items.query().next()) {
                itemsIds.push_back(items.query().value(0));
            }

            SelectQueryBuilder<Part> parts;
            parts.addValueCondition(Part::pimItemIdFullColumnName(), Query::In, QVariant::fromValue(itemsIds));
            parts.addValueCondition(Part::storageFullColumnName(), Query::Equals, (int) Part::External);
            if (parts.exec()) {
                const auto partsList = parts.result();
                for (const auto &part : partsList) {
                    bool exists = false;
                    const auto filename = ExternalPartStorage::resolveAbsolutePath(part.data(), &exists);
                    if (exists) {
                        QFile::remove(filename);
                    }
                }
            }

            items = QueryBuilder(PimItem::tableName(), QueryBuilder::Delete);
            items.addCondition(condition);
            if (!items.exec()) {
                inform(QStringLiteral("Error while deleting duplicates ") + items.query().lastError().text());
            }
        }
    }
}

void StorageJanitor::vacuum()
{
    const DbType::Type dbType = DbType::type(DataStore::self()->database());
    if (dbType == DbType::MySQL || dbType == DbType::PostgreSQL) {
        inform("vacuuming database, that'll take some time and require a lot of temporary disk space...");
        Q_FOREACH (const QString &table, allDatabaseTables()) {
            inform(QStringLiteral("optimizing table %1...").arg(table));

            QString queryStr;
            if (dbType == DbType::MySQL) {
                queryStr = QLatin1Literal("OPTIMIZE TABLE ") + table;
            } else if (dbType == DbType::PostgreSQL) {
                queryStr = QLatin1Literal("VACUUM FULL ANALYZE ") + table;
            } else {
                continue;
            }
            QSqlQuery q(DataStore::self()->database());
            if (!q.exec(queryStr)) {
                qCCritical(AKONADISERVER_LOG) << "failed to optimize table" << table << ":" << q.lastError().text();
            }
        }
        inform("vacuum done");
    } else {
        inform("Vacuum not supported for this database backend.");
    }

    Q_EMIT done();
}

void StorageJanitor::checkSizeTreshold()
{
    {
        QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
        qb.addColumn(Part::idFullColumnName());
        qb.addValueCondition(Part::storageFullColumnName(), Query::Equals, Part::Internal);
        qb.addValueCondition(Part::datasizeFullColumnName(), Query::Greater, DbConfig::configuredDatabase()->sizeThreshold());
        if (!qb.exec()) {
            inform("Failed to query parts larger than treshold, skipping test");
            return;
        }

        QSqlQuery query = qb.query();
        inform(QStringLiteral("Found %1 parts to be moved to external files").arg(query.size()));

        while (query.next()) {
            Transaction transaction(DataStore::self(), QStringLiteral("JANITOR CHECK SIZE THRESHOLD"));
            Part part = Part::retrieveById(query.value(0).toLongLong());
            const QByteArray name = ExternalPartStorage::nameForPartId(part.id());
            const QString partPath = ExternalPartStorage::resolveAbsolutePath(name);
            QFile f(partPath);
            if (f.exists()) {
                qCDebug(AKONADISERVER_LOG) << "External payload file" << name << "already exists";
                // That however is not a critical issue, since the part is not external,
                // so we can safely overwrite it
            }
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                qCCritical(AKONADISERVER_LOG) << "Failed to open file" << name << "for writing";
                continue;
            }
            if (f.write(part.data()) != part.datasize()) {
                qCCritical(AKONADISERVER_LOG) << "Failed to write data to payload file" << name;
                f.remove();
                continue;
            }

            part.setData(name);
            part.setStorage(Part::External);
            if (!part.update() || !transaction.commit()) {
                qCCritical(AKONADISERVER_LOG) << "Failed to update database entry of part" << part.id();
                f.remove();
                continue;
            }

            inform(QStringLiteral("Moved part %1 from database into external file %2").arg(part.id()).arg(QString::fromLatin1(name)));
        }
    }

    {
        QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
        qb.addColumn(Part::idFullColumnName());
        qb.addValueCondition(Part::storageFullColumnName(), Query::Equals, Part::External);
        qb.addValueCondition(Part::datasizeFullColumnName(), Query::Less, DbConfig::configuredDatabase()->sizeThreshold());
        if (!qb.exec()) {
            inform("Failed to query parts smaller than treshold, skipping test");
            return;
        }

        QSqlQuery query = qb.query();
        inform(QStringLiteral("Found %1 parts to be moved to database").arg(query.size()));

        while (query.next()) {
            Transaction transaction(DataStore::self(), QStringLiteral("JANITOR CHECK SIZE THRESHOLD 2"));
            Part part = Part::retrieveById(query.value(0).toLongLong());
            const QString partPath = ExternalPartStorage::resolveAbsolutePath(part.data());
            QFile f(partPath);
            if (!f.exists()) {
                qCCritical(AKONADISERVER_LOG) << "Part file" << part.data() << "does not exist";
                continue;
            }
            if (!f.open(QIODevice::ReadOnly)) {
                qCCritical(AKONADISERVER_LOG) << "Failed to open part file" << part.data() << "for reading";
                continue;
            }

            part.setStorage(Part::Internal);
            part.setData(f.readAll());
            if (part.data().size() != part.datasize()) {
                qCCritical(AKONADISERVER_LOG) << "Sizes of" << part.id() << "data don't match";
                continue;
            }
            if (!part.update() || !transaction.commit()) {
                qCCritical(AKONADISERVER_LOG) << "Failed to update database entry of part" << part.id();
                continue;
            }

            f.close();
            f.remove();
            inform(QStringLiteral("Moved part %1 from external file into database").arg(part.id()));
        }
    }
}

void StorageJanitor::migrateToLevelledCacheHierarchy()
{
    QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
    qb.addColumn(Part::idColumn());
    qb.addColumn(Part::dataColumn());
    qb.addValueCondition(Part::storageColumn(), Query::Equals, Part::External);
    if (!qb.exec()) {
        inform("Failed to query external payload parts, skipping test");
        return;
    }

    QSqlQuery query = qb.query();
    while (query.next()) {
        const qint64 id = query.value(0).toLongLong();
        const QByteArray data = query.value(1).toByteArray();
        const QString fileName = QString::fromUtf8(data);
        bool oldExists = false, newExists = false;
        // Resolve the current path
        const QString currentPath = ExternalPartStorage::resolveAbsolutePath(fileName, &oldExists);
        // Resolve the new path with legacy fallback disabled, so that it always
        // returns the new levelled-cache path, even when the old one exists
        const QString newPath = ExternalPartStorage::resolveAbsolutePath(fileName, &newExists, false);
        if (!oldExists) {
            qCCritical(AKONADISERVER_LOG) << "Old payload part does not exist, skipping part" << fileName;
            continue;
        }
        if (currentPath != newPath) {
            if (newExists) {
                qCCritical(AKONADISERVER_LOG) << "Part is in legacy location, but the destination file already exists, skipping part" << fileName;
                continue;
            }

            QFile f(currentPath);
            if (!f.rename(newPath)) {
                qCCritical(AKONADISERVER_LOG) << "Failed to move part from" << currentPath << " to " << newPath << ":" << f.errorString();
                continue;
            }
            inform(QStringLiteral("Migrated part %1 to new levelled cache").arg(id));
        }
    }
}

void StorageJanitor::findOrphanSearchIndexEntries()
{
    QueryBuilder qb(Collection::tableName(), QueryBuilder::Select);
    qb.addSortColumn(Collection::idColumn(), Query::Ascending);
    qb.addColumn(Collection::idColumn());
    qb.addColumn(Collection::isVirtualColumn());
    if (!qb.exec()) {
        inform("Failed to query collections, skipping test");
        return;
    }

    QDBusInterface iface(DBus::agentServiceName(QStringLiteral("akonadi_indexing_agent"), DBus::Agent),
                         QStringLiteral("/"),
                         QStringLiteral("org.freedesktop.Akonadi.Indexer"),
                         DBusConnectionPool::threadConnection());
    if (!iface.isValid()) {
        inform("Akonadi Indexing Agent is not running, skipping test");
        return;
    }

    QSqlQuery query = qb.query();
    while (query.next()) {
        const qint64 colId = query.value(0).toLongLong();
        // Skip virtual collections, they are not indexed
        if (query.value(1).toBool()) {
            inform(QStringLiteral("Skipping virtual Collection %1").arg(colId));
            continue;
        }

        inform(QStringLiteral("Checking Collection %1 search index...").arg(colId));
        SearchRequest req("StorageJanitor");
        req.setStoreResults(true);
        req.setCollections({ colId });
        req.setRemoteSearch(false);
        req.setQuery(QStringLiteral("{ }")); // empty query to match all
        QStringList mts;
        Collection col;
        col.setId(colId);
        const auto colMts = col.mimeTypes();
        if (colMts.isEmpty()) {
            // No mimetypes means we don't know which search store to look into,
            // skip it.
            continue;
        }
        mts.reserve(colMts.count());
        for (const auto &mt : colMts) {
            mts << mt.name();
        }
        req.setMimeTypes(mts);
        req.exec();
        auto searchResults = req.results();

        QueryBuilder iqb(PimItem::tableName(), QueryBuilder::Select);
        iqb.addColumn(PimItem::idColumn());
        iqb.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, colId);
        if (!iqb.exec()) {
            inform(QStringLiteral("Failed to query items in collection %1").arg(colId));
            continue;
        }

        QSqlQuery itemQuery = iqb.query();
        while (itemQuery.next()) {
            searchResults.remove(itemQuery.value(0).toLongLong());
        }

        if (!searchResults.isEmpty()) {
            inform(QStringLiteral("Collection %1 search index contains %2 orphan items. Scheduling reindexing").arg(colId).arg(searchResults.count()));
            iface.call(QDBus::NoBlock, QStringLiteral("reindexCollection"), colId);
        }
    }
}


void StorageJanitor::inform(const char *msg)
{
    inform(QLatin1String(msg));
}

void StorageJanitor::inform(const QString &msg)
{
    qCDebug(AKONADISERVER_LOG) << msg;
    Q_EMIT information(msg);
}
