/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "storagejanitor.h"
#include "agentmanagerinterface.h"
#include "akonadi.h"
#include "akonadiserver_debug.h"
#include "akranges.h"
#include "entities.h"
#include "resourcemanager.h"
#include "search/searchmanager.h"
#include "search/searchrequest.h"
#include "storage/collectionstatistics.h"
#include "storage/datastore.h"
#include "storage/dbconfig.h"
#include "storage/dbtype.h"
#include "storage/query.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include "private/dbus_p.h"
#include "private/externalpartstorage_p.h"
#include "private/standarddirs_p.h"

#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QSqlError>
#include <QSqlQuery>
#include <QStringBuilder>

#include <algorithm>
#include <functional>
#include <qregularexpression.h>

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

class StorageJanitorDataStore : public DataStore
{
public:
    StorageJanitorDataStore(AkonadiServer *server, DbConfig *config)
        : DataStore(server, config)
    {
    }
};

StorageJanitor::StorageJanitor(AkonadiServer *akonadi, DbConfig *dbConfig)
    : AkThread(QStringLiteral("StorageJanitor"), QThread::IdlePriority)
    , m_lostFoundCollectionId(-1)
    , m_akonadi(akonadi)
    , m_dbConfig(dbConfig)
{
}

StorageJanitor::StorageJanitor(DbConfig *dbConfig)
    : AkThread(QStringLiteral("StorageJanitor"), AkThread::NoThread)
    , m_lostFoundCollectionId(-1)
    , m_akonadi(nullptr)
    , m_dbConfig(dbConfig)
{
    StorageJanitor::init();
}

StorageJanitor::~StorageJanitor()
{
    quitThread();
}

void StorageJanitor::init()
{
    AkThread::init();

    registerTasks();

    m_dataStore = std::make_unique<StorageJanitorDataStore>(m_akonadi, m_dbConfig);
    m_dataStore->open();

    QDBusConnection conn = QDBusConnection::sessionBus();
    conn.registerService(DBus::serviceName(DBus::StorageJanitor));
    conn.registerObject(QStringLiteral(AKONADI_DBUS_STORAGEJANITOR_PATH),
                        this,
                        QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals);
}

void StorageJanitor::quit()
{
    QDBusConnection conn = QDBusConnection::sessionBus();
    conn.unregisterObject(QStringLiteral(AKONADI_DBUS_STORAGEJANITOR_PATH), QDBusConnection::UnregisterTree);
    conn.unregisterService(DBus::serviceName(DBus::StorageJanitor));

    // Make sure all children are deleted within context of this thread
    qDeleteAll(children());

    m_dataStore->close();

    AkThread::quit();
}

void StorageJanitor::registerTasks()
{
    m_tasks = {{QStringLiteral("Looking for collections not belonging to a valid resource..."), &StorageJanitor::findOrphanedCollections},
               {QStringLiteral("Checking collection tree consistency..."), &StorageJanitor::checkCollectionTreeConsistency},
               {QStringLiteral("Looking for items not belonging to a valid collection..."), &StorageJanitor::findOrphanedItems},
               {QStringLiteral("Looking for item parts not belonging to a valid item..."), &StorageJanitor::findOrphanedParts},
               {QStringLiteral("Looking for item flags not belonging to a valid item..."), &StorageJanitor::findOrphanedPimItemFlags},
               {QStringLiteral("Looking for duplicate item flags..."), &StorageJanitor::findDuplicateFlags},
               {QStringLiteral("Looking for duplicate mime types..."), &StorageJanitor::findDuplicateMimeTypes},
               {QStringLiteral("Looking for duplicate part types..."), &StorageJanitor::findDuplicatePartTypes},
               {QStringLiteral("Looking for duplicate tag types..."), &StorageJanitor::findDuplicateTagTypes},
               {QStringLiteral("Looking for duplicate relation types..."), &StorageJanitor::findDuplicateRelationTypes},
               {QStringLiteral("Looking for overlapping external parts..."), &StorageJanitor::findOverlappingParts},
               {QStringLiteral("Verifying external parts..."), &StorageJanitor::verifyExternalParts},
               {QStringLiteral("Checking size threshold changes..."), &StorageJanitor::checkSizeTreshold},
               {QStringLiteral("Looking for dirty objects..."), &StorageJanitor::findDirtyObjects},
               {QStringLiteral("Looking for rid-duplicates not matching the content mime-type of the parent collection"), &StorageJanitor::findRIDDuplicates},
               {QStringLiteral("Migrating parts to new cache hierarchy..."), &StorageJanitor::migrateToLevelledCacheHierarchy},
               {QStringLiteral("Making sure virtual search resource and collections exist"), &StorageJanitor::ensureSearchCollection}};

    // Tasks that require a valid Akonadi instance
    if (m_akonadi) {
        m_tasks += {{QStringLiteral("Looking for resources in the DB not matching a configured resource..."), &StorageJanitor::findOrphanedResources},
                    {QStringLiteral("Checking search index consistency..."), &StorageJanitor::findOrphanSearchIndexEntries},
                    {QStringLiteral("Flushing collection statistics memory cache..."), &StorageJanitor::expireCollectionStatisticsCache}};
    }

    /* TODO some ideas for further checks:
     * the collection tree is non-cyclic
     * content type constraints of collections are not violated
     * find unused flags
     * find unused mimetypes
     * check for dead entries in relation tables
     * check if part size matches file size
     */
}

void StorageJanitor::check() // implementation of `akonadictl fsck`
{
    m_lostFoundCollectionId = -1; // start with a fresh one each time

    for (const auto &[idx, task] : m_tasks | Views::enumerate(1)) {
        inform(QStringLiteral("%1/%2 %3").arg(idx, 2).arg(m_tasks.size()).arg(task.name));
        std::invoke(task.func, this);
    }

    inform("Consistency check done.");

    Q_EMIT done();
}

qint64 StorageJanitor::lostAndFoundCollection()
{
    if (m_lostFoundCollectionId > 0) {
        return m_lostFoundCollectionId;
    }

    Transaction transaction(m_dataStore.get(), QStringLiteral("JANITOR LOST+FOUND"));
    Resource lfRes = Resource::retrieveByName(m_dataStore.get(), QStringLiteral("akonadi_lost+found_resource"));
    if (!lfRes.isValid()) {
        lfRes.setName(QStringLiteral("akonadi_lost+found_resource"));
        if (!lfRes.insert(m_dataStore.get())) {
            qCCritical(AKONADISERVER_LOG) << "Failed to create lost+found resource!";
        }
    }

    Collection lfRoot;
    SelectQueryBuilder<Collection> qb(m_dataStore.get());
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
        if (!lfRoot.insert(m_dataStore.get())) {
            qCCritical(AKONADISERVER_LOG) << "Failed to create lost+found root.";
        }
        if (m_akonadi) {
            m_dataStore->notificationCollector()->collectionAdded(lfRoot, lfRes.name().toUtf8());
        }
    }

    Collection lfCol;
    lfCol.setName(QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    lfCol.setResourceId(lfRes.id());
    lfCol.setParentId(lfRoot.id());
    if (!lfCol.insert(m_dataStore.get())) {
        qCCritical(AKONADISERVER_LOG) << "Failed to create lost+found collection!";
    }

    const auto retrieveAll = MimeType::retrieveAll(m_dataStore.get());
    for (const MimeType &mt : retrieveAll) {
        lfCol.addMimeType(m_dataStore.get(), mt);
    }

    if (m_akonadi) {
        m_dataStore->notificationCollector()->collectionAdded(lfCol, lfRes.name().toUtf8());
    }

    transaction.commit();
    m_lostFoundCollectionId = lfCol.id();
    return m_lostFoundCollectionId;
}

void StorageJanitor::findOrphanedResources()
{
    SelectQueryBuilder<Resource> qbres(m_dataStore.get());
    OrgFreedesktopAkonadiAgentManagerInterface iface(DBus::serviceName(DBus::Control), QStringLiteral("/AgentManager"), QDBusConnection::sessionBus(), this);
    if (!iface.isValid()) {
        inform(QStringLiteral("ERROR: Couldn't talk to %1").arg(DBus::Control));
        return;
    }
    const QStringList knownResources = iface.agentInstances();
    if (knownResources.isEmpty()) {
        inform(QStringLiteral("ERROR: no known resources. This must be a mistake?"));
        return;
    }
    qbres.addValueCondition(Resource::nameFullColumnName(), Query::NotIn, QVariant(knownResources));
    qbres.addValueCondition(Resource::idFullColumnName(), Query::NotEquals, 1); // skip akonadi_search_resource
    if (!qbres.exec()) {
        inform("Failed to query known resources, skipping test");
        return;
    }
    // qCDebug(AKONADISERVER_LOG) << "SQL:" << qbres.query().lastQuery();
    const Resource::List orphanResources = qbres.result();
    const int orphanResourcesSize(orphanResources.size());
    if (orphanResourcesSize > 0) {
        QStringList resourceNames;
        resourceNames.reserve(orphanResourcesSize);
        for (const Resource &resource : orphanResources) {
            resourceNames.append(resource.name());
        }
        inform(QStringLiteral("Found %1 orphan resources: %2").arg(orphanResourcesSize).arg(resourceNames.join(QLatin1Char(','))));
        for (const QString &resourceName : std::as_const(resourceNames)) {
            inform(QStringLiteral("Removing resource %1").arg(resourceName));
            m_akonadi->resourceManager().removeResourceInstance(resourceName);
        }
    }
}

void StorageJanitor::findOrphanedCollections()
{
    SelectQueryBuilder<Collection> qb(m_dataStore.get());
    qb.addJoin(QueryBuilder::LeftJoin, Resource::tableName(), Collection::resourceIdFullColumnName(), Resource::idFullColumnName());
    qb.addValueCondition(Resource::idFullColumnName(), Query::Is, QVariant());

    if (!qb.exec()) {
        inform("Failed to query orphaned collections, skipping test");
        return;
    }
    const Collection::List orphans = qb.result();
    if (!orphans.isEmpty()) {
        inform(QLatin1StringView("Found ") + QString::number(orphans.size()) + QLatin1StringView(" orphan collections."));
        // TODO: attach to lost+found resource
    }
}

void StorageJanitor::checkCollectionTreeConsistency()
{
    const Collection::List cols = Collection::retrieveAll(m_dataStore.get());
    std::for_each(cols.begin(), cols.end(), [this](const Collection &col) {
        checkPathToRoot(col);
    });
}

void StorageJanitor::checkPathToRoot(const Collection &col)
{
    if (col.parentId() == 0) {
        return;
    }
    const Collection parent = col.parent(m_dataStore.get());
    if (!parent.isValid()) {
        inform(QLatin1StringView("Collection \"") + col.name() + QLatin1StringView("\" (id: ") + QString::number(col.id())
               + QLatin1StringView(") has no valid parent."));
        // TODO fix that by attaching to a top-level lost+found folder
        return;
    }

    if (col.resourceId() != parent.resourceId()) {
        inform(QLatin1StringView("Collection \"") + col.name() + QLatin1StringView("\" (id: ") + QString::number(col.id())
               + QLatin1StringView(") belongs to a different resource than its parent."));
        // can/should we actually fix that?
    }

    checkPathToRoot(parent);
}

void StorageJanitor::findOrphanedItems()
{
    SelectQueryBuilder<PimItem> qb(m_dataStore.get());
    qb.addJoin(QueryBuilder::LeftJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName());
    qb.addValueCondition(Collection::idFullColumnName(), Query::Is, QVariant());
    if (!qb.exec()) {
        inform("Failed to query orphaned items, skipping test");
        return;
    }
    const PimItem::List orphans = qb.result();
    if (!orphans.isEmpty()) {
        inform(QLatin1StringView("Found ") + QString::number(orphans.size()) + QLatin1StringView(" orphan items."));
        // Attach to lost+found collection
        Transaction transaction(m_dataStore.get(), QStringLiteral("JANITOR ORPHANS"));
        QueryBuilder qb(m_dataStore.get(), PimItem::tableName(), QueryBuilder::Update);
        qint64 col = lostAndFoundCollection();
        if (col == -1) {
            return;
        }
        qb.setColumnValue(PimItem::collectionIdColumn(), col);
        qb.addValueCondition(PimItem::idFullColumnName(),
                             Query::In,
                             orphans | Views::transform([](const auto &item) {
                                 return item.id();
                             }) | Actions::toQList);
        if (qb.exec() && transaction.commit()) {
            inform(QLatin1StringView("Moved orphan items to collection ") + QString::number(col));
        } else {
            inform(QLatin1StringView("Error moving orphan items to collection ") + QString::number(col) + QLatin1StringView(" : ")
                   + qb.query().lastError().text());
        }
    }
}

void StorageJanitor::findOrphanedParts()
{
    SelectQueryBuilder<Part> qb(m_dataStore.get());
    qb.addJoin(QueryBuilder::LeftJoin, PimItem::tableName(), Part::pimItemIdFullColumnName(), PimItem::idFullColumnName());
    qb.addValueCondition(PimItem::idFullColumnName(), Query::Is, QVariant());
    if (!qb.exec()) {
        inform("Failed to query orphaned parts, skipping test");
        return;
    }
    const Part::List orphans = qb.result();
    if (!orphans.isEmpty()) {
        inform(QLatin1StringView("Found ") + QString::number(orphans.size()) + QLatin1StringView(" orphan parts."));
        // TODO: create lost+found items for those? delete?
    }
}

void StorageJanitor::findOrphanedPimItemFlags()
{
    QueryBuilder sqb(m_dataStore.get(), PimItemFlagRelation::tableName(), QueryBuilder::Select);
    sqb.addColumn(PimItemFlagRelation::leftFullColumnName());
    sqb.addJoin(QueryBuilder::LeftJoin, PimItem::tableName(), PimItemFlagRelation::leftFullColumnName(), PimItem::idFullColumnName());
    sqb.addValueCondition(PimItem::idFullColumnName(), Query::Is, QVariant());
    if (!sqb.exec()) {
        inform("Failed to query orphaned item flags, skipping test");
        return;
    }

    QList<PimItem::Id> ids;
    while (sqb.query().next()) {
        ids.append(sqb.query().value(0).toInt());
    }
    sqb.query().finish();
    if (!ids.empty()) {
        QueryBuilder qb(m_dataStore.get(), PimItemFlagRelation::tableName(), QueryBuilder::Delete);
        qb.addValueCondition(PimItemFlagRelation::leftFullColumnName(), Query::In, ids);
        if (!qb.exec()) {
            qCCritical(AKONADISERVER_LOG) << "Error:" << qb.query().lastError().text();
            return;
        }

        inform(QLatin1StringView("Found and deleted ") + QString::number(ids.size()) + QLatin1StringView(" orphan pim item flags."));
    }
}

struct RelationDesc {
    QString tableName;
    QString deduplEntityIdColumnName;
};

template<typename DeduplEntity>
std::optional<int> findDuplicatesImpl(DataStore *dataStore, const QString &nameColumn, const RelationDesc &relation)
{
    QueryBuilder sqb(dataStore, DeduplEntity::tableName(), QueryBuilder::Select);
    sqb.addColumns({DeduplEntity::idColumn(), nameColumn});
    sqb.addSortColumn(DeduplEntity::idColumn());
    if (!sqb.exec()) {
        return std::nullopt;
    }

    QMap<QString, QVariantList> duplicates;
    while (sqb.query().next()) {
        const auto id = sqb.query().value(0).toLongLong();
        const auto name = sqb.query().value(1).toString();

        auto it = duplicates.find(name.trimmed());
        if (it == duplicates.end()) {
            it = duplicates.insert(name.trimmed(), QVariantList{});
        }
        it->push_back(id);
    }

    int removed = 0;
    for (const auto &[duplicateName, duplicateIds] : duplicates.asKeyValueRange()) {
        if (duplicateIds.size() <= 1) {
            // Not duplicated
            continue;
        }

        Transaction transaction(dataStore, QStringLiteral("StorageJanitor deduplicate %1 %2").arg(DeduplEntity::tableName(), duplicateName));

        // Update all relations with duplicated entity to use the lowest entity ID, so we can remove the
        // duplicates afterwards
        const auto firstId = duplicateIds.takeFirst();

        QueryBuilder updateQb(dataStore, relation.tableName, QueryBuilder::Update);
        updateQb.setColumnValue(relation.deduplEntityIdColumnName, firstId);
        updateQb.addValueCondition(relation.deduplEntityIdColumnName, Query::In, duplicateIds);
        if (!updateQb.exec()) {
            continue;
        }

        // Remove the duplicated entities
        QueryBuilder removeQb(dataStore, DeduplEntity::tableName(), QueryBuilder::Delete);
        removeQb.addValueCondition(DeduplEntity::idColumn(), Query::In, duplicateIds);
        if (!removeQb.exec()) {
            continue;
        }

        ++removed;

        transaction.commit();
    }

    return removed;
}

void StorageJanitor::findDuplicateFlags()
{
    const auto removed =
        findDuplicatesImpl<Flag>(m_dataStore.get(), Flag::nameFullColumnName(), {PimItemFlagRelation::tableName(), PimItemFlagRelation::rightFullColumnName()});
    if (removed) {
        inform(u"Removed " % QString::number(*removed) % u" duplicate item flags");
    } else {
        inform("Error while trying to remove duplicate Flags");
    }
}

void StorageJanitor::findDuplicateMimeTypes()
{
    const auto removed =
        findDuplicatesImpl<MimeType>(m_dataStore.get(), MimeType::nameFullColumnName(), {PimItem::tableName(), PimItem::mimeTypeIdFullColumnName()});
    if (removed) {
        inform(u"Removed " % QString::number(*removed) % u" duplicate mime types");
    } else {
        inform("Error while trying to remove duplicate MimeTypes");
    }
}

void StorageJanitor::findDuplicatePartTypes()
{
    // Good thing that SQL is ANSI/ISO standardized...
    QString nameColumn;
    if (DbType::type(m_dataStore->database()) == DbType::MySQL) {
        nameColumn = QStringLiteral("CONCAT_WS(':', %1, %2) AS name");
    } else {
        nameColumn = QStringLiteral("(%1 || ':' || %2) AS name");
    }

    const auto removed = findDuplicatesImpl<PartType>(m_dataStore.get(),
                                                      nameColumn.arg(PartType::nsFullColumnName(), PartType::nameFullColumnName()),
                                                      {Part::tableName(), Part::partTypeIdFullColumnName()});
    if (removed) {
        inform(u"Removed " % QString::number(*removed) % u" duplicate part types");
    } else {
        inform("Error while trying to remove duplicate PartTypes");
    }
}

void StorageJanitor::findDuplicateTagTypes()
{
    const auto removed = findDuplicatesImpl<TagType>(m_dataStore.get(), TagType::nameFullColumnName(), {Tag::tableName(), Tag::typeIdFullColumnName()});
    if (removed) {
        inform(u"Removed " % QString::number(*removed) % u" duplicate tag types");
    } else {
        inform("Error while trying to remove duplicate TagTypes");
    }
}

void StorageJanitor::findDuplicateRelationTypes()
{
    const auto removed =
        findDuplicatesImpl<RelationType>(m_dataStore.get(), RelationType::nameFullColumnName(), {Relation::tableName(), Relation::typeIdFullColumnName()});
    if (removed) {
        inform(u"Removed " % QString::number(*removed) % u" duplicate relation types");
    } else {
        inform("Error while trying to remove duplicate RelationTypes");
    }
}

void StorageJanitor::findOverlappingParts()
{
    QueryBuilder qb(m_dataStore.get(), Part::tableName(), QueryBuilder::Select);
    qb.addColumn(Part::dataColumn());
    qb.addColumn(QLatin1StringView("count(") + Part::idColumn() + QLatin1StringView(") as cnt"));
    qb.addValueCondition(Part::storageColumn(), Query::Equals, Part::External);
    qb.addValueCondition(Part::dataColumn(), Query::IsNot, QVariant());
    qb.addGroupColumn(Part::dataColumn());
    qb.addValueCondition(QLatin1StringView("count(") + Part::idColumn() + QLatin1StringView(")"), Query::Greater, 1, QueryBuilder::HavingCondition);
    if (!qb.exec()) {
        inform("Failed to query overlapping parts, skipping test");
        return;
    }

    int count = 0;
    while (qb.query().next()) {
        ++count;
        inform(QLatin1StringView("Found overlapping part data: ") + qb.query().value(0).toString());
        // TODO: uh oh, this is bad, how do we recover from that?
    }
    qb.query().finish();

    if (count > 0) {
        inform(QLatin1StringView("Found ") + QString::number(count) + QLatin1StringView(" overlapping parts - bad."));
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
    existingFiles.remove(dataDir + QDir::separator() + QLatin1StringView(".."));
    inform(QLatin1StringView("Found ") + QString::number(existingFiles.size()) + QLatin1StringView(" external files."));

    // list all parts from the db which claim to have an associated file
    QueryBuilder qb(m_dataStore.get(), Part::tableName(), QueryBuilder::Select);
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
        const auto filename = qb.query().value(0).toByteArray();
        const auto pimItemId = qb.query().value(1).value<Entity::Id>();
        const auto partId = qb.query().value(2).value<Entity::Id>();
        QString partPath;
        if (!filename.isEmpty()) {
            partPath = ExternalPartStorage::resolveAbsolutePath(filename);
        } else {
            partPath = ExternalPartStorage::resolveAbsolutePath(ExternalPartStorage::nameForPartId(partId));
        }
        if (existingFiles.contains(partPath)) {
            usedFiles.insert(partPath);
        } else {
            inform(QLatin1StringView("Cleaning up missing external file: ") + partPath + QLatin1StringView(" for item: ") + QString::number(pimItemId)
                   + QLatin1StringView(" on part: ") + QString::number(partId));

            Part part;
            part.setId(partId);
            part.setPimItemId(pimItemId);
            part.setData(QByteArray());
            part.setDatasize(0);
            part.setStorage(Part::Internal);
            part.update(m_dataStore.get());
        }
    }
    qb.query().finish();
    inform(QLatin1StringView("Found ") + QString::number(usedFiles.size()) + QLatin1StringView(" external parts."));

    // see what's left and move it to lost+found
    const QSet<QString> unreferencedFiles = existingFiles - usedFiles;
    if (!unreferencedFiles.isEmpty()) {
        const QString lfDir = StandardDirs::saveDir("data", QStringLiteral("file_lost+found"));
        for (const QString &file : unreferencedFiles) {
            inform(QLatin1StringView("Found unreferenced external file: ") + file);
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
    SelectQueryBuilder<Collection> cqb(m_dataStore.get());
    cqb.setSubQueryMode(Query::Or);
    cqb.addValueCondition(Collection::remoteIdColumn(), Query::Is, QVariant());
    cqb.addValueCondition(Collection::remoteIdColumn(), Query::Equals, QString());
    if (!cqb.exec()) {
        inform("Failed to query collections without RID, skipping test");
        return;
    }
    const Collection::List ridLessCols = cqb.result();
    for (const Collection &col : ridLessCols) {
        inform(QLatin1StringView("Collection \"") + col.name() + QLatin1StringView("\" (id: ") + QString::number(col.id())
               + QLatin1StringView(") has no RID."));
    }
    inform(QLatin1StringView("Found ") + QString::number(ridLessCols.size()) + QLatin1StringView(" collections without RID."));

    SelectQueryBuilder<PimItem> iqb1(m_dataStore.get());
    iqb1.setSubQueryMode(Query::Or);
    iqb1.addValueCondition(PimItem::remoteIdColumn(), Query::Is, QVariant());
    iqb1.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, QString());
    if (!iqb1.exec()) {
        inform("Failed to query items without RID, skipping test");
        return;
    }
    const PimItem::List ridLessItems = iqb1.result();
    for (const PimItem &item : ridLessItems) {
        inform(QLatin1StringView("Item \"") + QString::number(item.id()) + QLatin1StringView("\" in collection \"") + QString::number(item.collectionId())
               + QLatin1StringView("\" has no RID."));
    }
    inform(QLatin1StringView("Found ") + QString::number(ridLessItems.size()) + QLatin1StringView(" items without RID."));

    SelectQueryBuilder<PimItem> iqb2(m_dataStore.get());
    iqb2.addValueCondition(PimItem::dirtyColumn(), Query::Equals, true);
    iqb2.addValueCondition(PimItem::remoteIdColumn(), Query::IsNot, QVariant());
    iqb2.addSortColumn(PimItem::idFullColumnName());
    if (!iqb2.exec()) {
        inform("Failed to query dirty items, skipping test");
        return;
    }
    const PimItem::List dirtyItems = iqb2.result();
    for (const PimItem &item : dirtyItems) {
        inform(QLatin1StringView("Item \"") + QString::number(item.id()) + QLatin1StringView("\" has RID and is dirty."));
    }
    inform(QLatin1StringView("Found ") + QString::number(dirtyItems.size()) + QLatin1StringView(" dirty items."));
}

void StorageJanitor::findRIDDuplicates()
{
    QueryBuilder qb(m_dataStore.get(), Collection::tableName(), QueryBuilder::Select);
    qb.addColumn(Collection::idColumn());
    qb.addColumn(Collection::nameColumn());
    qb.exec();

    while (qb.query().next()) {
        const auto colId = qb.query().value(0).value<Collection::Id>();
        const QString name = qb.query().value(1).toString();
        inform(QStringLiteral("Checking ") + name);

        QueryBuilder duplicates(m_dataStore.get(), PimItem::tableName(), QueryBuilder::Select);
        duplicates.addColumn(PimItem::remoteIdColumn());
        duplicates.addColumn(QStringLiteral("count(") + PimItem::idColumn() + QStringLiteral(") as cnt"));
        duplicates.addValueCondition(PimItem::remoteIdColumn(), Query::IsNot, QVariant());
        duplicates.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, colId);
        duplicates.addGroupColumn(PimItem::remoteIdColumn());
        duplicates.addValueCondition(QStringLiteral("count(") + PimItem::idColumn() + QLatin1Char(')'), Query::Greater, 1, QueryBuilder::HavingCondition);
        duplicates.exec();

        Akonadi::Server::Collection col = Akonadi::Server::Collection::retrieveById(m_dataStore.get(), colId);
        const QList<Akonadi::Server::MimeType> contentMimeTypes = col.mimeTypes(m_dataStore.get());
        QVariantList contentMimeTypesVariantList;
        contentMimeTypesVariantList.reserve(contentMimeTypes.count());
        for (const Akonadi::Server::MimeType &mimeType : contentMimeTypes) {
            contentMimeTypesVariantList << mimeType.id();
        }
        while (duplicates.query().next()) {
            const QString rid = duplicates.query().value(0).toString();

            Query::Condition condition(Query::And);
            condition.addValueCondition(PimItem::remoteIdColumn(), Query::Equals, rid);
            condition.addValueCondition(PimItem::mimeTypeIdColumn(), Query::NotIn, contentMimeTypesVariantList);
            condition.addValueCondition(PimItem::collectionIdColumn(), Query::Equals, colId);

            QueryBuilder items(m_dataStore.get(), PimItem::tableName(), QueryBuilder::Select);
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
            items.query().finish();
            if (itemsIds.isEmpty()) {
                // the mimetype filter may have dropped some entries from the
                // duplicates query
                continue;
            }

            inform(QStringLiteral("Found duplicates ") + rid);

            SelectQueryBuilder<Part> parts(m_dataStore.get());
            parts.addValueCondition(Part::pimItemIdFullColumnName(), Query::In, QVariant::fromValue(itemsIds));
            parts.addValueCondition(Part::storageFullColumnName(), Query::Equals, static_cast<int>(Part::External));
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

            items = QueryBuilder(m_dataStore.get(), PimItem::tableName(), QueryBuilder::Delete);
            items.addCondition(condition);
            if (!items.exec()) {
                inform(QStringLiteral("Error while deleting duplicates ") + items.query().lastError().text());
            }
        }
        duplicates.query().finish();
    }
    qb.query().finish();
}

void StorageJanitor::vacuum()
{
    const DbType::Type dbType = DbType::type(m_dataStore->database());
    if (dbType == DbType::MySQL || dbType == DbType::PostgreSQL) {
        inform("vacuuming database, that'll take some time and require a lot of temporary disk space...");
        const auto tables = allDatabaseTables();
        for (const QString &table : tables) {
            inform(QStringLiteral("optimizing table %1...").arg(table));

            QString queryStr;
            if (dbType == DbType::MySQL) {
                queryStr = QLatin1StringView("OPTIMIZE TABLE ") + table;
            } else if (dbType == DbType::PostgreSQL) {
                queryStr = QLatin1StringView("VACUUM FULL ANALYZE ") + table;
            } else {
                continue;
            }
            QSqlQuery q(m_dataStore->database());
            if (!q.exec(queryStr)) {
                qCCritical(AKONADISERVER_LOG) << "failed to optimize table" << table << ":" << q.lastError().text();
            }
        }
        inform("vacuum done");
    } else {
        inform("Vacuum not supported for this database backend. (Sqlite backend)");
    }

    Q_EMIT done();
}

void StorageJanitor::checkSizeTreshold()
{
    {
        QueryBuilder qb(m_dataStore.get(), Part::tableName(), QueryBuilder::Select);
        qb.addColumn(Part::idFullColumnName());
        qb.addValueCondition(Part::storageFullColumnName(), Query::Equals, Part::Internal);
        qb.addValueCondition(Part::datasizeFullColumnName(), Query::Greater, m_dbConfig->sizeThreshold());
        if (!qb.exec()) {
            inform("Failed to query parts larger than threshold, skipping test");
            return;
        }

        QSqlQuery query = qb.query();
        inform(QStringLiteral("Found %1 parts to be moved to external files").arg(query.size()));

        while (query.next()) {
            Transaction transaction(m_dataStore.get(), QStringLiteral("JANITOR CHECK SIZE THRESHOLD"));
            Part part = Part::retrieveById(m_dataStore.get(), query.value(0).toLongLong());
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
            if (!part.update(m_dataStore.get()) || !transaction.commit()) {
                qCCritical(AKONADISERVER_LOG) << "Failed to update database entry of part" << part.id();
                f.remove();
                continue;
            }

            inform(QStringLiteral("Moved part %1 from database into external file %2").arg(part.id()).arg(QString::fromLatin1(name)));
        }
        query.finish();
    }

    {
        QueryBuilder qb(m_dataStore.get(), Part::tableName(), QueryBuilder::Select);
        qb.addColumn(Part::idFullColumnName());
        qb.addValueCondition(Part::storageFullColumnName(), Query::Equals, Part::External);
        qb.addValueCondition(Part::datasizeFullColumnName(), Query::Less, DbConfig::configuredDatabase()->sizeThreshold());
        if (!qb.exec()) {
            inform("Failed to query parts smaller than threshold, skipping test");
            return;
        }

        QSqlQuery query = qb.query();
        inform(QStringLiteral("Found %1 parts to be moved to database").arg(query.size()));

        while (query.next()) {
            Transaction transaction(m_dataStore.get(), QStringLiteral("JANITOR CHECK SIZE THRESHOLD 2"));
            Part part = Part::retrieveById(m_dataStore.get(), query.value(0).toLongLong());
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
            if (!part.update(m_dataStore.get()) || !transaction.commit()) {
                qCCritical(AKONADISERVER_LOG) << "Failed to update database entry of part" << part.id();
                continue;
            }

            f.close();
            f.remove();
            inform(QStringLiteral("Moved part %1 from external file into database").arg(part.id()));
        }
        query.finish();
    }
}

void StorageJanitor::migrateToLevelledCacheHierarchy()
{
    /// First, check whether that's still necessary
    const QString db_data = StandardDirs::saveDir("data", QStringLiteral("file_db_data"));
    const auto entries = QDir(db_data).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    if (entries.isEmpty()) {
        inform("No external parts in legacy location, skipping migration");
        return;
    }

    QueryBuilder qb(m_dataStore.get(), Part::tableName(), QueryBuilder::Select);
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
        bool oldExists = false;
        bool newExists = false;
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
    query.finish();
}

void StorageJanitor::findOrphanSearchIndexEntries()
{
    QueryBuilder qb(m_dataStore.get(), Collection::tableName(), QueryBuilder::Select);
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
                         QDBusConnection::sessionBus());
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
        SearchRequest req("StorageJanitor", m_akonadi->searchManager(), m_akonadi->agentSearchManager());
        req.setStoreResults(true);
        req.setCollections({colId});
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

        QueryBuilder iqb(m_dataStore.get(), PimItem::tableName(), QueryBuilder::Select);
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
        itemQuery.finish();

        if (!searchResults.isEmpty()) {
            inform(QStringLiteral("Collection %1 search index contains %2 orphan items. Scheduling reindexing").arg(colId).arg(searchResults.count()));
            iface.call(QDBus::NoBlock, QStringLiteral("reindexCollection"), colId);
        }
    }
    query.finish();
}

void StorageJanitor::ensureSearchCollection()
{
    static const auto searchResourceName = QStringLiteral("akonadi_search_resource");

    auto searchResource = Resource::retrieveByName(m_dataStore.get(), searchResourceName);
    if (!searchResource.isValid()) {
        searchResource.setName(searchResourceName);
        searchResource.setIsVirtual(true);
        if (!searchResource.insert(m_dataStore.get())) {
            inform(QStringLiteral("Failed to create Search resource."));
            return;
        }
    }

    auto searchCols = Collection::retrieveFiltered(m_dataStore.get(), Collection::resourceIdColumn(), searchResource.id());
    if (searchCols.isEmpty()) {
        Collection searchCol;
        searchCol.setId(1);
        searchCol.setName(QStringLiteral("Search"));
        searchCol.setResource(searchResource);
        searchCol.setIndexPref(Collection::False);
        searchCol.setIsVirtual(true);
        if (!searchCol.insert(m_dataStore.get())) {
            inform(QStringLiteral("Failed to create Search Collection"));
            return;
        }
    }
}

void StorageJanitor::expireCollectionStatisticsCache()
{
    m_akonadi->collectionStatistics().expireCache();
}

void StorageJanitor::inform(const char *msg)
{
    inform(QLatin1StringView(msg));
}

void StorageJanitor::inform(const QString &msg)
{
    qCDebug(AKONADISERVER_LOG) << msg;
    Q_EMIT information(msg);
}

#include "moc_storagejanitor.cpp"
