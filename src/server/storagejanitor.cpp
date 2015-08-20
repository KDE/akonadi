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
#include "resourcemanager.h"
#include "entities.h"
#include "dbusconnectionpool.h"
#include "agentmanagerinterface.h"

#include <shared/akdbus.h>
#include <shared/akdebug.h>

#include <private/imapset_p.h>
#include <private/protocol_p.h>
#include <private/standarddirs_p.h>


#include <QStringBuilder>
#include <QtDBus/QDBusConnection>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <QtCore/QDir>
#include <QtCore/qdiriterator.h>
#include <QDateTime>

#include <algorithm>

using namespace Akonadi;
using namespace Akonadi::Server;

StorageJanitorThread::StorageJanitorThread(QObject *parent)
    : QThread(parent)
{
}

void StorageJanitorThread::run()
{
    StorageJanitor *janitor = new StorageJanitor;
    exec();
    delete janitor;
}

StorageJanitor::StorageJanitor(QObject *parent)
    : QObject(parent)
    , m_connection(DBusConnectionPool::threadConnection())
    , m_lostFoundCollectionId(-1)
{
    DataStore::self();
    m_connection.registerService(AkDBus::serviceName(AkDBus::StorageJanitor));
    m_connection.registerObject(QStringLiteral(AKONADI_DBUS_STORAGEJANITOR_PATH), this, QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);
}

StorageJanitor::~StorageJanitor()
{
    m_connection.unregisterObject(QStringLiteral(AKONADI_DBUS_STORAGEJANITOR_PATH), QDBusConnection::UnregisterTree);
    m_connection.unregisterService(AkDBus::serviceName(AkDBus::StorageJanitor));
    m_connection.disconnectFromBus(m_connection.name());

    DataStore::self()->close();
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
    std::for_each(cols.begin(), cols.end(), [this](const Collection &col) { checkPathToRoot(col); });

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

    /* TODO some ideas for further checks:
     * the collection tree is non-cyclic
     * content type constraints of collections are not violated
     * find unused flags
     * find unused mimetypes
     * check for dead entries in relation tables
     * check if part size matches file size
     */

    inform("Consistency check done.");
}

qint64 StorageJanitor::lostAndFoundCollection()
{
    if (m_lostFoundCollectionId > 0) {
        return m_lostFoundCollectionId;
    }

    Transaction transaction(DataStore::self());
    Resource lfRes = Resource::retrieveByName(QStringLiteral("akonadi_lost+found_resource"));
    if (!lfRes.isValid()) {
        lfRes.setName(QStringLiteral("akonadi_lost+found_resource"));
        if (!lfRes.insert()) {
            akFatal() << "Failed to create lost+found resource!";
        }
    }

    Collection lfRoot;
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition(Collection::resourceIdFullColumnName(), Query::Equals, lfRes.id());
    qb.addValueCondition(Collection::parentIdFullColumnName(), Query::Is, QVariant());
    qb.exec();
    const Collection::List cols = qb.result();
    if (cols.size() > 1) {
        akFatal() << "More than one top-level lost+found collection!?";
    } else if (cols.size() == 1) {
        lfRoot = cols.first();
    } else {
        lfRoot.setName(QStringLiteral("lost+found"));
        lfRoot.setResourceId(lfRes.id());
        lfRoot.setCachePolicyLocalParts(QStringLiteral("ALL"));
        lfRoot.setCachePolicyCacheTimeout(-1);
        lfRoot.setCachePolicyInherit(false);
        if (!lfRoot.insert()) {
            akFatal() << "Failed to create lost+found root.";
        }
        DataStore::self()->notificationCollector()->collectionAdded(lfRoot, lfRes.name().toUtf8());
    }

    Collection lfCol;
    lfCol.setName(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    lfCol.setResourceId(lfRes.id());
    lfCol.setParentId(lfRoot.id());
    if (!lfCol.insert()) {
        akFatal() << "Failed to create lost+found collection!";
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
        AkDBus::serviceName(AkDBus::Control),
        QStringLiteral("/AgentManager"),
        QDBusConnection::sessionBus(),
        this);
    if (!iface.isValid()) {
        inform(QStringLiteral("ERROR: Couldn't talk to %1").arg(AkDBus::Control));
        return;
    }
    const QStringList knownResources = iface.agentInstances();
    if (knownResources.isEmpty()) {
        inform(QStringLiteral("ERROR: no known resources. This must be a mistake?"));
        return;
    }
    akDebug() << "Known resources:" << knownResources;
    qbres.addValueCondition(Resource::nameFullColumnName(), Query::NotIn, QVariant(knownResources));
    qbres.addValueCondition(Resource::idFullColumnName(), Query::NotEquals, 1);   // skip akonadi_search_resource
    qbres.exec();
    //akDebug() << "SQL:" << qbres.query().lastQuery();
    const Resource::List orphanResources = qbres.result();
    if (orphanResources.size() > 0) {
        QStringList resourceNames;
        Q_FOREACH (const Resource &resource, orphanResources) {
            resourceNames.append(resource.name());
        }
        inform(QStringLiteral("Found %1 orphan resources: %2").arg(orphanResources.size()). arg(resourceNames.join(QStringLiteral(","))));
        Q_FOREACH (const QString &resourceName, resourceNames) {
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

    qb.exec();
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
    qb.exec();
    const PimItem::List orphans = qb.result();
    if (orphans.size() > 0) {
        inform(QLatin1Literal("Found ") + QString::number(orphans.size()) + QLatin1Literal(" orphan items."));
        // Attach to lost+found collection
        Transaction transaction(DataStore::self());
        QueryBuilder qb(PimItem::tableName(), QueryBuilder::Update);
        qint64 col = lostAndFoundCollection();
        qb.setColumnValue(PimItem::collectionIdFullColumnName(), col);
        QVector<ImapSet::Id> imapIds;
        imapIds.reserve(orphans.count());
        Q_FOREACH (const PimItem &item, orphans) {
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

    qb.exec();
    const Part::List orphans = qb.result();
    if (orphans.size() > 0) {
        inform(QLatin1Literal("Found ") + QString::number(orphans.size()) + QLatin1Literal(" orphan parts."));
        // TODO: create lost+found items for those? delete?
    }
}

void StorageJanitor::findOrphanedPimItemFlags()
{
    QueryBuilder sqb(PimItemFlagRelation::tableName(), QueryBuilder::Select);
    sqb.addColumn(PimItemFlagRelation::leftFullColumnName());
    sqb.addJoin(QueryBuilder::LeftJoin, PimItem::tableName(), PimItemFlagRelation::leftFullColumnName(), PimItem::idFullColumnName());
    sqb.addValueCondition(PimItem::idFullColumnName(), Query::Is, QVariant());
    if (!sqb.exec()) {
        akError() << "Error:" << sqb.query().lastError().text();
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
            akError() << "Error:" << qb.query().lastError().text();
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
    qb.addValueCondition(Part::externalColumn(), Query::Equals, true);
    qb.addValueCondition(Part::dataColumn(), Query::IsNot, QVariant());
    qb.addGroupColumn(Part::dataColumn());
    qb.addValueCondition(QLatin1Literal("count(") + Part::idColumn() + QLatin1Literal(")"), Query::Greater, 1, QueryBuilder::HavingCondition);
    qb.exec();

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
    QDirIterator it(dataDir);
    while (it.hasNext()) {
        existingFiles.insert(it.next());
    }
    existingFiles.remove(dataDir + QDir::separator() + QLatin1String("."));
    existingFiles.remove(dataDir + QDir::separator() + QLatin1String(".."));
    inform(QLatin1Literal("Found ") + QString::number(existingFiles.size()) + QLatin1Literal(" external files."));

    // list all parts from the db which claim to have an associated file
    QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
    qb.addColumn(Part::dataColumn());
    qb.addColumn(Part::pimItemIdColumn());
    qb.addColumn(Part::idColumn());
    qb.addValueCondition(Part::externalColumn(), Query::Equals, true);
    qb.addValueCondition(Part::dataColumn(), Query::IsNot, QVariant());
    qb.exec();
    while (qb.query().next()) {
        QString partPath = PartHelper::resolveAbsolutePath(qb.query().value(0).toByteArray());
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
            part.setExternal(false);
            part.update();
        }
    }
    inform(QLatin1Literal("Found ") + QString::number(usedFiles.size()) + QLatin1Literal(" external parts."));

    // see what's left and move it to lost+found
    const QSet<QString> unreferencedFiles = existingFiles - usedFiles;
    if (!unreferencedFiles.isEmpty()) {
        const QString lfDir = StandardDirs::saveDir("data", QStringLiteral("file_lost+found"));
        Q_FOREACH (const QString &file, unreferencedFiles) {
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
    cqb.exec();
    const Collection::List ridLessCols = cqb.result();
    Q_FOREACH (const Collection &col, ridLessCols) {
        inform(QLatin1Literal("Collection \"") + col.name() + QLatin1Literal("\" (id: ") + QString::number(col.id())
               + QLatin1Literal(") has no RID."));
    }
    inform(QLatin1Literal("Found ") + QString::number(ridLessCols.size()) + QLatin1Literal(" collections without RID."));

    SelectQueryBuilder<PimItem> iqb1;
    iqb1.setSubQueryMode(Query::Or);
    iqb1.addValueCondition(PimItem::remoteIdColumn(), Query::Is, QVariant());
    iqb1.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, QString());
    iqb1.exec();
    const PimItem::List ridLessItems = iqb1.result();
    Q_FOREACH (const PimItem &item, ridLessItems) {
        inform(QLatin1Literal("Item \"") + QString::number(item.id()) + QLatin1Literal("\" has no RID."));
    }
    inform(QLatin1Literal("Found ") + QString::number(ridLessItems.size()) + QLatin1Literal(" items without RID."));

    SelectQueryBuilder<PimItem> iqb2;
    iqb2.addValueCondition(PimItem::dirtyColumn(), Query::Equals, true);
    iqb2.addValueCondition(PimItem::remoteIdColumn(), Query::IsNot, QVariant());
    iqb2.addSortColumn(PimItem::idFullColumnName());
    iqb2.exec();
    const PimItem::List dirtyItems = iqb2.result();
    Q_FOREACH (const PimItem &item, dirtyItems) {
        inform(QLatin1Literal("Item \"") + QString::number(item.id()) + QLatin1Literal("\" has RID and is dirty."));
    }
    inform(QLatin1Literal("Found ") + QString::number(dirtyItems.size()) + QLatin1Literal(" dirty items."));
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
                akError() << "failed to optimize table" << table << ":" << q.lastError().text();
            }
        }
        inform("vacuum done");
    } else {
        inform("Vacuum not supported for this database backend.");
    }
}

void StorageJanitor::checkSizeTreshold()
{
    {
        QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
        qb.addColumn(Part::idFullColumnName());
        qb.addValueCondition(Part::externalFullColumnName(), Query::Equals, false);
        qb.addValueCondition(Part::datasizeFullColumnName(), Query::Greater, DbConfig::configuredDatabase()->sizeThreshold());
        qb.exec();

        QSqlQuery query = qb.query();
        inform(QStringLiteral("Found %1 parts to be moved to external files").arg(query.size()));

        while (query.next()) {
            Transaction transaction(DataStore::self());
            Part part = Part::retrieveById(query.value(0).toLongLong());
            const QByteArray name = PartHelper::fileNameForPart(&part).toUtf8() + "_r" + QByteArray::number(part.version());
            const QString partPath = PartHelper::resolveAbsolutePath(name);
            QFile f(partPath);
            if (f.exists()) {
                akDebug() << "External payload file" << name << "already exists";
                // That however is not a critical issue, since the part is not external,
                // so we can safely overwrite it
            }
            if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                akError() << "Failed to open file" << name << "for writing";
                continue;
            }
            if (f.write(part.data()) != part.datasize()) {
                akError() << "Failed to write data to payload file" << name;
                f.remove();
                continue;
            }

            part.setData(name);
            part.setExternal(true);
            if (!part.update() || !transaction.commit()) {
                akError() << "Failed to update database entry of part" << part.id();
                f.remove();
                continue;
            }

            inform(QStringLiteral("Moved part %1 from database into external file %2").arg(part.id()).arg(QString::fromLatin1(name)));
        }
    }

    {
        QueryBuilder qb(Part::tableName(), QueryBuilder::Select);
        qb.addColumn(Part::idFullColumnName());
        qb.addValueCondition(Part::externalFullColumnName(), Query::Equals, true);
        qb.addValueCondition(Part::datasizeFullColumnName(), Query::Less, DbConfig::configuredDatabase()->sizeThreshold());
        qb.exec();

        QSqlQuery query = qb.query();
        inform(QStringLiteral("Found %1 parts to be moved to database").arg(query.size()));

        while (query.next()) {
            Transaction transaction(DataStore::self());
            Part part = Part::retrieveById(query.value(0).toLongLong());
            const QString partPath = PartHelper::resolveAbsolutePath(part.data());
            QFile f(partPath);
            if (!f.exists()) {
                akError() << "Part file" << part.data() << "does not exist";
                continue;
            }
            if (!f.open(QIODevice::ReadOnly)) {
                akError() << "Failed to open part file" << part.data() << "for reading";
                continue;
            }

            part.setExternal(false);
            part.setData(f.readAll());
            if (part.data().size() != part.datasize()) {
                akError() << "Sizes of" << part.id() << "data don't match";
                continue;
            }
            if (!part.update() || !transaction.commit()) {
                akError() << "Failed to update database entry of part" << part.id();
                continue;
            }

            f.close();
            f.remove();
            inform(QStringLiteral("Moved part %1 from external file into database").arg(part.id()));
        }
    }
}

void StorageJanitor::inform(const char *msg)
{
    inform(QLatin1String(msg));
}

void StorageJanitor::inform(const QString &msg)
{
    akDebug() << msg;
    Q_EMIT information(msg);
}
