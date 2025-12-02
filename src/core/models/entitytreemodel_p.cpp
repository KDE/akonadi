/*
    SPDX-FileCopyrightText: 2008 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitytreemodel_p.h"

#include "agentmanagerinterface.h"
#include "akranges.h"
#include "monitor_p.h" // For friend ref/deref
#include "servermanager.h"

#include <KLocalizedString>

#include "agentmanager.h"
#include "agenttype.h"
#include "changerecorder.h"
#include "collectioncopyjob.h"
#include "collectionfetchscope.h"
#include "collectionmovejob.h"
#include "collectionstatistics.h"
#include "entityhiddenattribute.h"
#include "itemcopyjob.h"
#include "itemfetchjob.h"
#include "itemmovejob.h"
#include "linkjob.h"
#include "monitor.h"
#include "private/protocol_p.h"
#include "session.h"

#include "akonadicore_debug.h"

#include <QElapsedTimer>
#include <QIcon>
#include <unordered_map>

// clazy:excludeall=old-style-connect

QHash<KJob *, QElapsedTimer> jobTimeTracker;

Q_LOGGING_CATEGORY(DebugETM, "org.kde.pim.akonadi.ETM", QtInfoMsg)

using namespace Akonadi;
using namespace AkRanges;

static CollectionFetchJob::Type getFetchType(EntityTreeModel::CollectionFetchStrategy strategy)
{
    switch (strategy) {
    case EntityTreeModel::FetchFirstLevelChildCollections:
        return CollectionFetchJob::FirstLevel;
    case EntityTreeModel::FetchCollectionsMerged:
    case EntityTreeModel::FetchCollectionsRecursive:
    default:
        break;
    }
    return CollectionFetchJob::Recursive;
}

EntityTreeModelPrivate::EntityTreeModelPrivate(EntityTreeModel *parent)
    : q_ptr(parent)
{
    // using collection as a parameter of a queued call in runItemFetchJob()
    qRegisterMetaType<Collection>();

    Akonadi::AgentManager *agentManager = Akonadi::AgentManager::self();
    QObject::connect(agentManager, SIGNAL(instanceRemoved(Akonadi::AgentInstance)), q_ptr, SLOT(agentInstanceRemoved(Akonadi::AgentInstance)));
}

EntityTreeModelPrivate::~EntityTreeModelPrivate()
{
    if (m_needDeleteRootNode) {
        delete m_rootNode;
    }
    m_rootNode = nullptr;
}

void EntityTreeModelPrivate::init(Monitor *monitor)
{
    Q_Q(EntityTreeModel);
    Q_ASSERT(!m_monitor);
    m_monitor = monitor;
    // The default is to FetchCollectionsRecursive, so we tell the monitor to fetch collections
    // That way update signals from the monitor will contain the full collection.
    // This may be updated if the CollectionFetchStrategy is changed.
    m_monitor->fetchCollection(true);
    m_session = m_monitor->session();

    m_rootCollectionDisplayName = QStringLiteral("[*]");

    if (auto cr = qobject_cast<Akonadi::ChangeRecorder *>(m_monitor)) {
        cr->setChangeRecordingEnabled(false);
    }

    m_includeStatistics = true;
    m_monitor->fetchCollectionStatistics(true);
    m_monitor->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);

    q->connect(monitor, SIGNAL(mimeTypeMonitored(QString, bool)), SLOT(monitoredMimeTypeChanged(QString, bool)));
    q->connect(monitor, SIGNAL(collectionMonitored(Akonadi::Collection, bool)), SLOT(monitoredCollectionsChanged(Akonadi::Collection, bool)));
    q->connect(monitor, SIGNAL(itemMonitored(Akonadi::Item, bool)), SLOT(monitoredItemsChanged(Akonadi::Item, bool)));
    q->connect(monitor, SIGNAL(resourceMonitored(QByteArray, bool)), SLOT(monitoredResourcesChanged(QByteArray, bool)));

    // monitor collection changes
    q->connect(monitor, SIGNAL(collectionChanged(Akonadi::Collection)), SLOT(monitoredCollectionChanged(Akonadi::Collection)));
    q->connect(monitor,
               SIGNAL(collectionAdded(Akonadi::Collection, Akonadi::Collection)),
               SLOT(monitoredCollectionAdded(Akonadi::Collection, Akonadi::Collection)));
    q->connect(monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), SLOT(monitoredCollectionRemoved(Akonadi::Collection)));
    q->connect(monitor,
               SIGNAL(collectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)),
               SLOT(monitoredCollectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)));

    // Monitor item changes.
    q->connect(monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)), SLOT(monitoredItemAdded(Akonadi::Item, Akonadi::Collection)));
    q->connect(monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)), SLOT(monitoredItemChanged(Akonadi::Item, QSet<QByteArray>)));
    q->connect(monitor, SIGNAL(itemRemoved(Akonadi::Item)), SLOT(monitoredItemRemoved(Akonadi::Item)));
    q->connect(monitor,
               SIGNAL(itemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)),
               SLOT(monitoredItemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)));

    q->connect(monitor, SIGNAL(itemLinked(Akonadi::Item, Akonadi::Collection)), SLOT(monitoredItemLinked(Akonadi::Item, Akonadi::Collection)));
    q->connect(monitor, SIGNAL(itemUnlinked(Akonadi::Item, Akonadi::Collection)), SLOT(monitoredItemUnlinked(Akonadi::Item, Akonadi::Collection)));

    q->connect(monitor,
               SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id, Akonadi::CollectionStatistics)),
               SLOT(monitoredCollectionStatisticsChanged(Akonadi::Collection::Id, Akonadi::CollectionStatistics)));

    Akonadi::ServerManager *serverManager = Akonadi::ServerManager::self();
    q->connect(serverManager, SIGNAL(started()), SLOT(serverStarted()));

    fillModel();
}

void EntityTreeModelPrivate::prependNode(Node *node)
{
    m_childEntities[node->parent].prepend(node);
}

void EntityTreeModelPrivate::appendNode(Node *node)
{
    m_childEntities[node->parent].append(node);
}

void EntityTreeModelPrivate::serverStarted()
{
    // Don't emit about to be reset. Too late for that
    endResetModel();
}

void EntityTreeModelPrivate::changeFetchState(const Collection &parent)
{
    Q_Q(EntityTreeModel);
    const QModelIndex collectionIndex = indexForCollection(parent);
    if (!collectionIndex.isValid()) {
        // Because we are called delayed, it is possible that @p parent has been deleted.
        return;
    }
    Q_EMIT q->dataChanged(collectionIndex, collectionIndex);
}

void EntityTreeModelPrivate::agentInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    Q_Q(EntityTreeModel);
    if (!instance.type().capabilities().contains(QLatin1StringView("Resource"))) {
        return;
    }

    if (m_rootCollection.isValid()) {
        if (m_rootCollection != Collection::root()) {
            if (m_rootCollection.resource() == instance.identifier()) {
                q->clearAndReset();
            }
            return;
        }
        const auto children = m_childEntities[Collection::root().id()];
        for (const Node *node : children) {
            Q_ASSERT(node->type == Node::Collection);

            const Collection collection = m_collections[node->id];
            if (collection.resource() == instance.identifier()) {
                monitoredCollectionRemoved(collection);
            }
        }
    }
}

static const char s_fetchCollectionId[] = "FetchCollectionId";

void EntityTreeModelPrivate::fetchItems(const Collection &parent)
{
    Q_Q(const EntityTreeModel);
    Q_ASSERT(parent.isValid());
    Q_ASSERT(m_collections.contains(parent.id()));
    // TODO: Use a more specific fetch scope to get only the envelope for mails etc.
    auto itemFetchJob = new Akonadi::ItemFetchJob(parent, m_session);
    itemFetchJob->setFetchScope(m_monitor->itemFetchScope());
    itemFetchJob->fetchScope().setAncestorRetrieval(ItemFetchScope::All);
    itemFetchJob->fetchScope().setIgnoreRetrievalErrors(true);
    itemFetchJob->setDeliveryOption(ItemFetchJob::EmitItemsInBatches);

    itemFetchJob->setProperty(s_fetchCollectionId, QVariant(parent.id()));

    if (m_showRootCollection || parent != m_rootCollection) {
        m_pendingCollectionRetrieveJobs.insert(parent.id());

        // If collections are not in the model, there will be no valid index for them.
        if (m_collectionFetchStrategy != EntityTreeModel::FetchCollectionsMerged && //
            m_collectionFetchStrategy != EntityTreeModel::FetchNoCollections) {
            // We need to invoke this delayed because we would otherwise be emitting a sequence like
            // - beginInsertRows
            // - dataChanged
            // - endInsertRows
            // which would confuse proxies.
            QMetaObject::invokeMethod(const_cast<EntityTreeModel *>(q), "changeFetchState", Qt::QueuedConnection, Q_ARG(Akonadi::Collection, parent));
        }
    }

    q->connect(itemFetchJob, &ItemFetchJob::itemsReceived, q, [this, parentId = parent.id()](const Item::List &items) {
        itemsFetched(parentId, items);
    });
    q->connect(itemFetchJob, &ItemFetchJob::result, q, [this, parentId = parent.id()](KJob *job) {
        itemFetchJobDone(parentId, job);
    });
    qCDebug(DebugETM) << "collection:" << parent.name();
    jobTimeTracker[itemFetchJob].start();
}

void EntityTreeModelPrivate::fetchCollections(Akonadi::CollectionFetchJob *job)
{
    Q_Q(EntityTreeModel);

    job->fetchScope().setListFilter(m_listFilter);
    job->fetchScope().setContentMimeTypes(m_monitor->mimeTypesMonitored());
    m_pendingCollectionFetchJobs.insert(static_cast<KJob *>(job));

    if (m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsMerged) {
        // This doesn't display items's collections, so no model signals are emitted about the collections themselves
        q->connect(job, &CollectionFetchJob::collectionsReceived, q, [this](const Collection::List &collections) {
            for (const auto &collection : collections) {
                if (isHidden(collection)) {
                    continue;
                }
                m_collections.insert(collection.id(), collection);
                prependNode(new Node{Node::Collection, collection.id(), -1});
                fetchItems(collection);
            }
        });
    } else {
        job->fetchScope().setIncludeStatistics(m_includeStatistics);
        job->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), q, SLOT(collectionsFetched(Akonadi::Collection::List)));
    }
    q->connect(job, SIGNAL(result(KJob *)), q, SLOT(collectionFetchJobDone(KJob *)));

    jobTimeTracker[job].start();
}

void EntityTreeModelPrivate::fetchCollections(const Collection::List &collections, CollectionFetchJob::Type type)
{
    fetchCollections(new CollectionFetchJob(collections, type, m_session));
}

void EntityTreeModelPrivate::fetchCollections(const Collection &collection, CollectionFetchJob::Type type)
{
    Q_ASSERT(collection.isValid());
    auto job = new CollectionFetchJob(collection, type, m_session);
    fetchCollections(job);
}

namespace Akonadi
{
template<typename T>
inline bool EntityTreeModelPrivate::isHiddenImpl(const T &entity, Node::Type type) const
{
    if (m_showSystemEntities) {
        return false;
    }

    if (type == Node::Collection && entity.id() == m_rootCollection.id()) {
        return false;
    }

    // entity.hasAttribute<EntityHiddenAttribute>() does not compile w/ GCC for
    // some reason
    if (entity.hasAttribute(EntityHiddenAttribute().type())) {
        return true;
    }

    const Collection parent = entity.parentCollection();
    if (parent.isValid()) {
        return isHiddenImpl(parent, Node::Collection);
    }

    return false;
}

} // namespace Akonadi

bool EntityTreeModelPrivate::isHidden(const Akonadi::Collection &collection) const
{
    return isHiddenImpl(collection, Node::Collection);
}

bool EntityTreeModelPrivate::isHidden(const Akonadi::Item &item) const
{
    return isHiddenImpl(item, Node::Item);
}

static QSet<Collection::Id> getChildren(Collection::Id parent, const std::unordered_map<Collection::Id, Collection::Id> &childParentMap)
{
    QSet<Collection::Id> children;
    for (const auto &[childId, parentId] : childParentMap) {
        if (parentId == parent) {
            children.insert(childId);
            children.unite(getChildren(childId, childParentMap));
        }
    }
    return children;
}

void EntityTreeModelPrivate::collectionsFetched(const Akonadi::Collection::List &collections)
{
    Q_Q(EntityTreeModel);
    QElapsedTimer t;
    t.start();

    QVectorIterator<Akonadi::Collection> it(collections);

    QHash<Collection::Id, Collection> collectionsToInsert;

    while (it.hasNext()) {
        const Collection collection = it.next();
        const Collection::Id collectionId = collection.id();
        if (isHidden(collection)) {
            continue;
        }

        auto collectionIt = m_collections.find(collectionId);
        if (collectionIt != m_collections.end()) {
            // This is probably the result of a parent of a previous collection already being in the model.
            // Replace the dummy collection with the real one and move on.

            // This could also be the result of a monitor signal having already inserted the collection
            // into this model. There's no way to tell, so we just emit dataChanged.
            *collectionIt = collection;

            const QModelIndex collectionIndex = indexForCollection(collection);
            dataChanged(collectionIndex, collectionIndex);
            Q_EMIT q->collectionFetched(collectionId);
            continue;
        }

        // If we're monitoring collections somewhere in the tree we need to retrieve their ancestors now
        if (collection.parentCollection() != m_rootCollection && m_monitor->collectionsMonitored().contains(collection)) {
            retrieveAncestors(collection, false);
        }

        collectionsToInsert.insert(collectionId, collection);
    }

    // Build a list of subtrees to insert, with the root of the subtree on the left, and the complete subtree including root on the right
    std::unordered_map<Collection::Id, QSet<Collection::Id>> subTreesToInsert;
    {
        // Build a child-parent map that allows us to build the subtrees afterwards
        std::unordered_map<Collection::Id, Collection::Id> childParentMap;
        const auto initialCollectionsToInsert(collectionsToInsert);
        for (const auto &col : initialCollectionsToInsert) {
            childParentMap.insert({col.id(), col.parentCollection().id()});

            // Complete the subtree up to the last known parent
            Collection parent = col.parentCollection();
            while (parent.isValid() && parent != m_rootCollection && !m_collections.contains(parent.id())) {
                childParentMap.insert({parent.id(), parent.parentCollection().id()});

                if (!collectionsToInsert.contains(parent.id())) {
                    collectionsToInsert.insert(parent.id(), parent);
                }
                parent = parent.parentCollection();
            }
        }

        QSet<Collection::Id> parents;

        // Find toplevel parents of the subtrees
        for (const auto &[childId, parentId] : childParentMap) {
            // The child has a parent without parent (it's a toplevel node that is not yet in m_collections)
            if (childParentMap.find(parentId) == childParentMap.cend()) {
                Q_ASSERT(!m_collections.contains(childId));
                parents.insert(childId);
            }
        }

        // Find children of each subtree
        for (const auto parentId : parents) {
            QSet<Collection::Id> children;
            // We add the parent itself as well so it can be inserted below as part of the same loop
            children << parentId;
            children += getChildren(parentId, childParentMap);
            subTreesToInsert.insert_or_assign(parentId, std::move(children));
        }
    }

    const int row = 0;

    for (const auto &[topCollectionId, subtree] : subTreesToInsert) {
        qCDebug(DebugETM) << "Subtree: " << topCollectionId << subtree;

        Q_ASSERT(!m_collections.contains(topCollectionId));
        Collection topCollection = collectionsToInsert.value(topCollectionId);
        Q_ASSERT(topCollection.isValid());

        // The toplevels parent must already be part of the model
        Q_ASSERT(m_collections.contains(topCollection.parentCollection().id()));
        const QModelIndex parentIndex = indexForCollection(topCollection.parentCollection());

        q->beginInsertRows(parentIndex, row, row);
        Q_ASSERT(!subtree.empty());

        for (const auto collectionId : subtree) {
            const Collection collection = collectionsToInsert.take(collectionId);
            Q_ASSERT(collection.isValid());

            m_collections.insert(collectionId, collection);

            Q_ASSERT(collection.parentCollection().isValid());
            prependNode(new Node{Node::Collection, collectionId, collection.parentCollection().id()});
        }
        q->endInsertRows();

        if (m_itemPopulation == EntityTreeModel::ImmediatePopulation) {
            for (const auto collectionId : subtree) {
                const auto col = m_collections.value(collectionId);
                if (!m_mimeChecker.hasWantedMimeTypes() || m_mimeChecker.isWantedCollection(col)) {
                    fetchItems(col);
                } else {
                    // Consider collections that don't contain relevant mimetypes to be populated
                    m_populatedCols.insert(collectionId);
                    Q_EMIT q_ptr->collectionPopulated(collectionId);
                    const auto idx = indexForCollection(Collection(collectionId));
                    Q_ASSERT(idx.isValid());
                    dataChanged(idx, idx);
                }
            }
        }
    }
}

// Used by entitytreemodeltest
void EntityTreeModelPrivate::itemsFetched(const Akonadi::Item::List &items)
{
    Q_Q(EntityTreeModel);
    const auto collectionId = q->sender()->property(s_fetchCollectionId).value<Collection::Id>();
    itemsFetched(collectionId, items);
}

void EntityTreeModelPrivate::itemsFetched(const Collection::Id collectionId, const Akonadi::Item::List &items)
{
    Q_Q(EntityTreeModel);

    if (!m_collections.contains(collectionId)) {
        qCWarning(AKONADICORE_LOG) << "Collection has been removed while fetching items";
        return;
    }

    const Collection collection = m_collections.value(collectionId);

    Q_ASSERT(collection.isValid());

    // if there are any items at all, remove from set of collections known to be empty
    if (!items.isEmpty()) {
        m_collectionsWithoutItems.remove(collectionId);
    }

    Item::List itemsToInsert;
    for (const auto &item : items) {
        if (isHidden(item)) {
            continue;
        }

        if ((!m_mimeChecker.hasWantedMimeTypes() || m_mimeChecker.isWantedItem(item))) {
            // When listing virtual collections we might get results for items which are already in
            // the model if their concrete collection has already been listed.
            // In that case the collectionId should be different though.

            // As an additional complication, new items might be both part of fetch job results and
            // part of monitor notifications. We only insert items which are not already in the model
            // considering their (possibly virtual) parent.
            bool isNewItem = true;
            auto itemIt = m_items.find(item.id());
            if (itemIt != m_items.end()) {
                const Akonadi::Collection::List parents = getParentCollections(item);
                for (const Akonadi::Collection &parent : parents) {
                    if (parent.id() == collectionId) {
                        qCWarning(AKONADICORE_LOG) << "Fetched an item which is already in the model, id=" << item.id() << "collection id=" << collectionId;
                        // Update it in case the revision changed;
                        itemIt->value.apply(item);
                        isNewItem = false;
                        break;
                    }
                }
            }

            if (isNewItem) {
                itemsToInsert << item;
            }
        }
    }

    if (!itemsToInsert.isEmpty()) {
        const bool useRootCollection = //
            m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsMerged || //
            m_collectionFetchStrategy == EntityTreeModel::FetchNoCollections;
        const Collection::Id destCollectionId = useRootCollection ? m_rootCollection.id() : collectionId;

        QList<Node *> &collectionEntities = m_childEntities[destCollectionId];
        const int startRow = collectionEntities.size();

        Q_ASSERT(m_collections.contains(destCollectionId));
        const QModelIndex parentIndex = indexForCollection(m_collections.value(destCollectionId));

        q->beginInsertRows(parentIndex, startRow, startRow + itemsToInsert.size() - 1);
        for (const Item &item : std::as_const(itemsToInsert)) {
            const Item::Id itemId = item.id();
            m_items.ref(itemId, item);

            collectionEntities.append(new Node{Node::Item, itemId, collectionId /* yes, original ID here */});
        }
        q->endInsertRows();
    }
}

void EntityTreeModelPrivate::monitoredMimeTypeChanged(const QString &mimeType, bool monitored)
{
    beginResetModel();
    if (monitored) {
        m_mimeChecker.addWantedMimeType(mimeType);
    } else {
        m_mimeChecker.removeWantedMimeType(mimeType);
    }
    endResetModel();
}

void EntityTreeModelPrivate::monitoredCollectionsChanged(const Akonadi::Collection &collection, bool monitored)
{
    if (monitored) {
        const CollectionFetchJob::Type fetchType = getFetchType(m_collectionFetchStrategy);
        fetchCollections(collection, CollectionFetchJob::Base);
        fetchCollections(collection, fetchType);
    } else {
        // If a collection is dereferenced and no longer explicitly monitored it might still match other filters
        if (!shouldBePartOfModel(collection)) {
            monitoredCollectionRemoved(collection);
        }
    }
}

void EntityTreeModelPrivate::monitoredItemsChanged(const Akonadi::Item &item, bool monitored)
{
    Q_UNUSED(item)
    Q_UNUSED(monitored)
    beginResetModel();
    endResetModel();
}

void EntityTreeModelPrivate::monitoredResourcesChanged(const QByteArray &resource, bool monitored)
{
    Q_UNUSED(resource)
    Q_UNUSED(monitored)
    beginResetModel();
    endResetModel();
}

bool EntityTreeModelPrivate::retrieveAncestors(const Akonadi::Collection &collection, bool insertBaseCollection)
{
    Q_Q(EntityTreeModel);

    Collection parentCollection = collection.parentCollection();

    Q_ASSERT(parentCollection.isValid());
    Q_ASSERT(parentCollection != Collection::root());

    Collection::List ancestors;

    while (parentCollection != Collection::root() && !m_collections.contains(parentCollection.id())) {
        // Put a temporary node in the tree later.
        ancestors.prepend(parentCollection);

        parentCollection = parentCollection.parentCollection();
        // If we got here through Collection added notification, the parent chain may be incomplete
        // and if the model is still populating or the collection belongs to a yet-unknown subtree
        // this will break here
        if (!parentCollection.isValid()) {
            break;
        }
    }
    // if m_rootCollection is Collection::root(), we always have common ancestor and do the retrieval
    // if we traversed up to Collection::root() but are looking at a subtree only (m_rootCollection != Collection::root())
    // we have no common ancestor, and we don't have to retrieve anything
    if (parentCollection == Collection::root() && m_rootCollection != Collection::root()) {
        return true;
    }

    if (ancestors.isEmpty() && !insertBaseCollection) {
        // Nothing to do, avoid emitting insert signals
        return true;
    }

    CollectionFetchJob *job = nullptr;
    // We were unable to reach the top of the tree due to an incomplete ancestor chain, we will have
    // to retrieve it from the server.
    if (!parentCollection.isValid()) {
        if (insertBaseCollection) {
            job = new CollectionFetchJob(collection, CollectionFetchJob::Recursive, m_session);
        } else {
            job = new CollectionFetchJob(collection.parentCollection(), CollectionFetchJob::Recursive, m_session);
        }
    } else if (!ancestors.isEmpty()) {
        // Fetch the real ancestors
        job = new CollectionFetchJob(ancestors, CollectionFetchJob::Base, m_session);
    }

    if (job) {
        job->fetchScope().setListFilter(m_listFilter);
        job->fetchScope().setIncludeStatistics(m_includeStatistics);
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), q, SLOT(ancestorsFetched(Akonadi::Collection::List)));
        q->connect(job, SIGNAL(result(KJob *)), q, SLOT(collectionFetchJobDone(KJob *)));
    }

    if (!parentCollection.isValid()) {
        // We can't proceed to insert the fake collections to complete the tree because
        // we do not have the complete ancestor chain. However, once the fetch job is
        // finished the tree will be populated accordingly.
        return false;
    }

    //  Q_ASSERT( parentCollection != m_rootCollection );
    const QModelIndex parent = indexForCollection(parentCollection);

    // Still prepending all collections for now.
    int row = 0;

    // Although we insert several Collections here, we only need to notify though the model
    // about the top-level one. The rest will be found automatically by the view.
    q->beginInsertRows(parent, row, row);

    for (const auto &ancestor : std::as_const(ancestors)) {
        Q_ASSERT(ancestor.parentCollection().isValid());
        m_collections.insert(ancestor.id(), ancestor);

        prependNode(new Node{Node::Collection, ancestor.id(), ancestor.parentCollection().id()});
    }

    if (insertBaseCollection) {
        m_collections.insert(collection.id(), collection);
        // Can't just use parentCollection because that doesn't necessarily refer to collection.
        prependNode(new Node{Node::Collection, collection.id(), collection.parentCollection().id()});
    }

    q->endInsertRows();

    return true;
}

void EntityTreeModelPrivate::ancestorsFetched(const Akonadi::Collection::List &collectionList)
{
    for (const Collection &collection : collectionList) {
        m_collections[collection.id()] = collection;

        const QModelIndex index = indexForCollection(collection);
        Q_ASSERT(index.isValid());
        dataChanged(index, index);
    }
}

void EntityTreeModelPrivate::insertCollection(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_ASSERT(collection.isValid());
    Q_ASSERT(parent.isValid());

    Q_Q(EntityTreeModel);

    const int row = 0;
    const QModelIndex parentIndex = indexForCollection(parent);
    q->beginInsertRows(parentIndex, row, row);
    m_collections.insert(collection.id(), collection);
    prependNode(new Node{Node::Collection, collection.id(), parent.id()});
    q->endInsertRows();
}

bool EntityTreeModelPrivate::hasChildCollection(const Collection &collection) const
{
    const auto &children = m_childEntities[collection.id()];
    for (const Node *node : children) {
        if (node->type == Node::Collection) {
            const Collection subcol = m_collections[node->id];
            if (shouldBePartOfModel(subcol)) {
                return true;
            }
        }
    }
    return false;
}

bool EntityTreeModelPrivate::isAncestorMonitored(const Collection &collection) const
{
    Akonadi::Collection parent = collection.parentCollection();
    while (parent.isValid()) {
        if (m_monitor->collectionsMonitored().contains(parent)) {
            return true;
        }
        parent = parent.parentCollection();
    }
    return false;
}

bool EntityTreeModelPrivate::shouldBePartOfModel(const Collection &collection) const
{
    if (isHidden(collection)) {
        return false;
    }

    // We want a parent collection if it has at least one child that matches the
    // wanted mimetype
    if (hasChildCollection(collection)) {
        return true;
    }

    // Explicitly monitored collection
    if (m_monitor->collectionsMonitored().contains(collection)) {
        return true;
    }

    // We're explicitly monitoring collections, but didn't match the filter
    if (!m_mimeChecker.hasWantedMimeTypes() && !m_monitor->collectionsMonitored().isEmpty()) {
        // The collection should be included if one of the parents is monitored
        return isAncestorMonitored(collection);
    }

    // Some collection trees contain multiple mimetypes. Even though server side filtering ensures we
    // only get the ones we're interested in from the job, we have to filter on collections received through signals too.
    if (m_mimeChecker.hasWantedMimeTypes() && !m_mimeChecker.isWantedCollection(collection)) {
        return false;
    }

    if (m_listFilter == CollectionFetchScope::Enabled) {
        if (!collection.enabled()) {
            return false;
        }
    } else if (m_listFilter == CollectionFetchScope::Display) {
        if (!collection.shouldList(Collection::ListDisplay)) {
            return false;
        }
    } else if (m_listFilter == CollectionFetchScope::Sync) {
        if (!collection.shouldList(Collection::ListSync)) {
            return false;
        }
    } else if (m_listFilter == CollectionFetchScope::Index) {
        if (!collection.shouldList(Collection::ListIndex)) {
            return false;
        }
    }

    return true;
}

void EntityTreeModelPrivate::removeChildEntities(Collection::Id collectionId)
{
    const QList<Node *> childList = m_childEntities.value(collectionId);
    for (const Node *node : childList) {
        if (node->type == Node::Item) {
            m_items.unref(node->id);
        } else {
            removeChildEntities(node->id);
            m_collections.remove(node->id);
            m_populatedCols.remove(node->id);
        }
    }

    qDeleteAll(m_childEntities.take(collectionId));
}

QStringList EntityTreeModelPrivate::childCollectionNames(const Collection &collection) const
{
    return m_childEntities[collection.id()] | Views::filter([](const Node *node) {
               return node->type == Node::Collection;
           })
        | Views::transform([this](const Node *node) {
               return m_collections.value(node->id).name();
           })
        | Actions::toQList;
}

void EntityTreeModelPrivate::monitoredCollectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    // If the resource is removed while populating the model with it, we might still
    // get some monitor signals. These stale/out-of-order signals can't be completely eliminated
    // in the akonadi server due to implementation details, so we also handle such signals in the model silently
    // in all the monitored slots.
    // Stephen Kelly, 28, July 2009

    // If a fetch job is started and a collection is added to akonadi after the fetch job is started, the
    // new collection will be added to the fetch job results. It will also be notified through the monitor.
    // We return early here in that case.
    if (m_collections.contains(collection.id())) {
        return;
    }

    // If the resource is explicitly monitored all other checks are skipped. topLevelCollectionsFetched still checks the hidden attribute.
    if (m_monitor->resourcesMonitored().contains(collection.resource().toUtf8()) && collection.parentCollection() == Collection::root()) {
        topLevelCollectionsFetched({collection});
        return;
    }

    if (!shouldBePartOfModel(collection)) {
        return;
    }

    if (!m_collections.contains(parent.id())) {
        // The collection we're interested in is contained in a collection we're not interested in.
        // We download the ancestors of the collection we're interested in to complete the tree.
        if (collection != Collection::root()) {
            if (!retrieveAncestors(collection)) {
                return;
            }
        }
    } else {
        insertCollection(collection, parent);
    }

    if (m_itemPopulation == EntityTreeModel::ImmediatePopulation) {
        fetchItems(collection);
    }
}

void EntityTreeModelPrivate::monitoredCollectionRemoved(const Akonadi::Collection &collection)
{
    // if an explicitly monitored collection is removed, we would also have to remove collections which were included to show it (as in the move case)
    if ((collection == m_rootCollection) || m_monitor->collectionsMonitored().contains(collection)) {
        beginResetModel();
        endResetModel();
        return;
    }

    Collection::Id parentId = collection.parentCollection().id();
    if (parentId < 0) {
        parentId = -1;
    }

    if (!m_collections.contains(parentId)) {
        return;
    }

    // This may be a signal for a collection we've already removed by removing its ancestor.
    // Or the collection may have been hidden.
    if (!m_collections.contains(collection.id())) {
        return;
    }

    Q_Q(EntityTreeModel);

    Q_ASSERT(m_childEntities.contains(parentId));

    const int row = indexOf(m_childEntities.value(parentId), Node::Collection, collection.id());
    Q_ASSERT(row >= 0);

    Q_ASSERT(m_collections.contains(parentId));
    const Collection parentCollection = m_collections.value(parentId);

    m_populatedCols.remove(collection.id());

    const QModelIndex parentIndex = indexForCollection(parentCollection);

    q->beginRemoveRows(parentIndex, row, row);
    // Delete all descendant collections and items.
    removeChildEntities(collection.id());
    // Remove deleted collection from its parent.
    delete m_childEntities[parentId].takeAt(row);
    // Remove deleted collection itself.
    m_collections.remove(collection.id());
    q->endRemoveRows();

    // After removing a collection, check whether it's parent should be removed too
    if (!shouldBePartOfModel(parentCollection)) {
        monitoredCollectionRemoved(parentCollection);
    }
}

void EntityTreeModelPrivate::monitoredCollectionMoved(const Akonadi::Collection &collection,
                                                      const Akonadi::Collection &sourceCollection,
                                                      const Akonadi::Collection &destCollection)
{
    if (isHidden(collection)) {
        return;
    }

    if (isHidden(sourceCollection)) {
        if (isHidden(destCollection)) {
            return;
        }

        monitoredCollectionAdded(collection, destCollection);
        return;
    } else if (isHidden(destCollection)) {
        monitoredCollectionRemoved(collection);
        return;
    }

    if (!m_collections.contains(collection.id())) {
        return;
    }

    if (m_monitor->collectionsMonitored().contains(collection)) {
        // if we don't reset here, we would have to make sure that destination collection is actually available,
        // and remove the sources parents if they were only included as parents of the moved collection
        beginResetModel();
        endResetModel();
        return;
    }

    Q_Q(EntityTreeModel);

    const QModelIndex srcParentIndex = indexForCollection(sourceCollection);
    const QModelIndex destParentIndex = indexForCollection(destCollection);

    Q_ASSERT(collection.parentCollection().isValid());
    Q_ASSERT(destCollection.isValid());
    Q_ASSERT(collection.parentCollection() == destCollection);

    const int srcRow = indexOf(m_childEntities.value(sourceCollection.id()), Node::Collection, collection.id());
    const int destRow = 0; // Prepend collections

    if (!q->beginMoveRows(srcParentIndex, srcRow, srcRow, destParentIndex, destRow)) {
        qCWarning(AKONADICORE_LOG) << "Cannot move collection" << collection.id() << " from collection" << sourceCollection.id() << "to" << destCollection.id();
        return;
    }

    Node *node = m_childEntities[sourceCollection.id()].takeAt(srcRow);
    // collection has the correct parentCollection etc. We need to set it on the
    // internal data structure to not corrupt things.
    m_collections.insert(collection.id(), collection);
    node->parent = destCollection.id();
    m_childEntities[destCollection.id()].prepend(node);
    q->endMoveRows();
}

void EntityTreeModelPrivate::monitoredCollectionChanged(const Akonadi::Collection &collection)
{
    if (!m_collections.contains(collection.id())) {
        // This can happen if
        // * we get a change notification after removing the collection.
        // * a collection of a non-monitored mimetype is changed elsewhere. Monitor does not
        //    filter by content mimetype of Collections so we get notifications for all of them.

        // We might match the filter now, retry adding the collection
        monitoredCollectionAdded(collection, collection.parentCollection());
        return;
    }

    if (!shouldBePartOfModel(collection)) {
        monitoredCollectionRemoved(collection);
        return;
    }

    m_collections[collection.id()] = collection;

    if (!m_showRootCollection && collection == m_rootCollection) {
        // If the root of the model is not Collection::root it might be modified.
        // But it doesn't exist in the accessible model structure, so we need to early return
        return;
    }

    const QModelIndex index = indexForCollection(collection);
    Q_ASSERT(index.isValid());
    dataChanged(index, index);
}

void EntityTreeModelPrivate::monitoredCollectionStatisticsChanged(Akonadi::Collection::Id id, const Akonadi::CollectionStatistics &statistics)
{
    if (!m_collections.contains(id)) {
        return;
    }

    m_collections[id].setStatistics(statistics);

    // if the item count becomes 0, add to set of collections we know to be empty
    // otherwise remove if in there
    if (statistics.count() == 0) {
        m_collectionsWithoutItems.insert(id);
    } else {
        m_collectionsWithoutItems.remove(id);
    }

    if (!m_showRootCollection && id == m_rootCollection.id()) {
        // If the root of the model is not Collection::root it might be modified.
        // But it doesn't exist in the accessible model structure, so we need to early return
        return;
    }

    const QModelIndex index = indexForCollection(m_collections[id]);
    dataChanged(index, index);
}

void EntityTreeModelPrivate::monitoredItemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_Q(EntityTreeModel);

    if (isHidden(item)) {
        return;
    }

    const Collection::Id collectionId = collection.id();
    const Item::Id itemId = item.id();
    const bool isMergedFetch = m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsMerged;

    if (!isMergedFetch && !m_collections.contains(collectionId)) {
        qCWarning(AKONADICORE_LOG) << "Got a stale 'added' notification for an item whose collection was already removed." << itemId << item.remoteId();
        return;
    }

    if (m_items.contains(itemId)) {
        return;
    }

    Q_ASSERT(isMergedFetch || m_collections.contains(collectionId));

    if (m_mimeChecker.hasWantedMimeTypes() && !m_mimeChecker.isWantedItem(item)) {
        return;
    }

    // Adding items to not yet populated collections would block fetchMore, resulting in only new items showing up in the collection
    // This is only a problem with lazy population, otherwise fetchMore is not used at all
    if ((m_itemPopulation == EntityTreeModel::LazyPopulation) && !m_populatedCols.contains(collectionId)) {
        return;
    }

    QList<Node *> &collectionEntities = m_childEntities[!isMergedFetch ? collectionId : m_rootCollection.id()];
    const int row = collectionEntities.size();

    QModelIndex parentIndex;
    if (!isMergedFetch) {
        parentIndex = indexForCollection(m_collections.value(collectionId));
    }

    q->beginInsertRows(parentIndex, row, row);
    m_items.ref(itemId, item);
    collectionEntities.append(new Node{Node::Item, itemId, collectionId});
    q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemRemoved(const Akonadi::Item &item, const Akonadi::Collection &parentCollection)
{
    Q_Q(EntityTreeModel);

    if (isHidden(item)) {
        return;
    }

    if ((m_itemPopulation == EntityTreeModel::LazyPopulation)
        && !m_populatedCols.contains(parentCollection.isValid() ? parentCollection.id() : item.parentCollection().id())) {
        return;
    }

    const Collection::List parents = getParentCollections(item);
    if (parents.isEmpty()) {
        return;
    }

    if (!m_items.contains(item.id())) {
        qCWarning(AKONADICORE_LOG) << "Got a stale 'removed' notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    for (const auto &collection : parents) {
        Q_ASSERT(m_collections.contains(collection.id()));
        Q_ASSERT(m_childEntities.contains(collection.id()));

        const int row = indexOf(m_childEntities.value(collection.id()), Node::Item, item.id());
        Q_ASSERT(row >= 0);

        const QModelIndex parentIndex = indexForCollection(m_collections.value(collection.id()));

        q->beginRemoveRows(parentIndex, row, row);
        m_items.unref(item.id());
        delete m_childEntities[collection.id()].takeAt(row);
        q->endRemoveRows();
    }
}

void EntityTreeModelPrivate::monitoredItemChanged(const Akonadi::Item &item, const QSet<QByteArray> & /*unused*/)
{
    if (isHidden(item)) {
        return;
    }

    if ((m_itemPopulation == EntityTreeModel::LazyPopulation) && !m_populatedCols.contains(item.parentCollection().id())) {
        return;
    }

    auto itemIt = m_items.find(item.id());
    if (itemIt == m_items.end()) {
        qCWarning(AKONADICORE_LOG) << "Got a stale 'changed' notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    itemIt->value.apply(item);
    // Notifications about itemChange are always dispatched for real collection
    // and also all virtual collections the item belongs to. In order to preserve
    // the original storage collection when we need to have special handling for
    // notifications for virtual collections
    if (item.parentCollection().isVirtual()) {
        const Collection originalParent = itemIt->value.parentCollection();
        itemIt->value.setParentCollection(originalParent);
    }

    const QModelIndexList indexes = indexesForItem(item);
    for (const QModelIndex &index : indexes) {
        if (index.isValid()) {
            dataChanged(index, index);
        } else {
            qCWarning(AKONADICORE_LOG) << "item has invalid index:" << item.id() << item.remoteId();
        }
    }
}

void EntityTreeModelPrivate::monitoredItemMoved(const Akonadi::Item &item,
                                                const Akonadi::Collection &sourceCollection,
                                                const Akonadi::Collection &destCollection)
{
    if (isHidden(item)) {
        return;
    }

    if (isHidden(sourceCollection)) {
        if (isHidden(destCollection)) {
            return;
        }

        monitoredItemAdded(item, destCollection);
        return;
    } else if (isHidden(destCollection)) {
        monitoredItemRemoved(item, sourceCollection);
        return;
    } else {
        monitoredItemRemoved(item, sourceCollection);
        monitoredItemAdded(item, destCollection);
        return;
    }
    // "Temporarily" commented out as it's likely the best course to
    // avoid the dreaded "reset storm" (or layoutChanged storm). The
    // whole itemMoved idea is great but not practical until all the
    // other proxy models play nicely with it, right now they just
    // transform moved signals in layout changed, which explodes into
    // a reset of the source model inside of the message list (ouch!)
#if 0
    if (!m_items.contains(item.id())) {
        qCWarning(AKONADICORE_LOG) << "Got a stale 'moved' notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    Q_ASSERT(m_collections.contains(sourceCollection.id()));
    Q_ASSERT(m_collections.contains(destCollection.id()));

    const QModelIndex srcIndex = indexForCollection(sourceCollection);
    const QModelIndex destIndex = indexForCollection(destCollection);

    // Where should it go? Always append items and prepend collections and reorganize them with separate reactions to Attributes?

    const Item::Id itemId = item.id();

    const int srcRow = indexOf(m_childEntities.value(sourceCollection.id()), Node::Item, itemId);
    const int destRow = q->rowCount(destIndex);

    Q_ASSERT(srcRow >= 0);
    Q_ASSERT(destRow >= 0);
    if (!q->beginMoveRows(srcIndex, srcRow, srcRow, destIndex, destRow)) {
        qCWarning(AKONADICORE_LOG) << "Invalid move";
        return;
    }

    Q_ASSERT(m_childEntities.contains(sourceCollection.id()));
    Q_ASSERT(m_childEntities[sourceCollection.id()].size() > srcRow);

    Node *node = m_childEntities[sourceCollection.id()].takeAt(srcRow);
    m_items.insert(item.id(), item);
    node->parent = destCollection.id();
    m_childEntities[destCollection.id()].append(node);
    q->endMoveRows();
#endif
}

void EntityTreeModelPrivate::monitoredItemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_Q(EntityTreeModel);

    if (isHidden(item)) {
        return;
    }

    const Collection::Id collectionId = collection.id();
    const Item::Id itemId = item.id();
    const bool isMergedFetch = m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsMerged;

    if (!isMergedFetch && !m_collections.contains(collectionId)) {
        qCWarning(AKONADICORE_LOG) << "Got a stale 'linked' notification for an item whose collection was already removed." << itemId << item.remoteId();
        return;
    }

    Q_ASSERT(isMergedFetch || m_collections.contains(collectionId));

    if (m_mimeChecker.hasWantedMimeTypes() && !m_mimeChecker.isWantedItem(item)) {
        return;
    }

    // Adding items to not yet populated collections would block fetchMore, resulting in only new items showing up in the collection
    // This is only a problem with lazy population, otherwise fetchMore is not used at all
    if ((m_itemPopulation == EntityTreeModel::LazyPopulation) && !m_populatedCols.contains(collectionId)) {
        return;
    }

    QList<Node *> &collectionEntities = m_childEntities[!isMergedFetch ? collectionId : m_rootCollection.id()];

    const int existingPosition = indexOf(collectionEntities, Node::Item, itemId);
    if (existingPosition > 0) {
        qCWarning(AKONADICORE_LOG) << "Item with id " << itemId << " already in virtual collection with id " << collectionId;
        return;
    }

    const int row = collectionEntities.size();

    QModelIndex parentIndex;
    if (!isMergedFetch) {
        parentIndex = indexForCollection(m_collections.value(collectionId));
    }

    q->beginInsertRows(parentIndex, row, row);
    m_items.ref(itemId, item);
    collectionEntities.append(new Node{Node::Item, itemId, collectionId});
    q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_Q(EntityTreeModel);

    if (isHidden(item)) {
        return;
    }

    if ((m_itemPopulation == EntityTreeModel::LazyPopulation) && !m_populatedCols.contains(item.parentCollection().id())) {
        return;
    }

    if (!m_items.contains(item.id())) {
        qCWarning(AKONADICORE_LOG) << "Got a stale 'unlinked' notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    Q_ASSERT(m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsMerged || m_collections.contains(collection.id()));

    const int row = indexOf(m_childEntities.value(collection.id()), Node::Item, item.id());
    if (row < 0 || row >= m_childEntities[collection.id()].size()) {
        qCWarning(AKONADICORE_LOG) << "couldn't find index of unlinked item " << item.id() << collection.id() << row;
        Q_ASSERT(false);
        return;
    }

    const QModelIndex parentIndex = indexForCollection(m_collections.value(collection.id()));

    q->beginRemoveRows(parentIndex, row, row);
    delete m_childEntities[collection.id()].takeAt(row);
    m_items.unref(item.id());
    q->endRemoveRows();
}

void EntityTreeModelPrivate::collectionFetchJobDone(KJob *job)
{
    m_pendingCollectionFetchJobs.remove(job);
    auto cJob = static_cast<CollectionFetchJob *>(job);
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Job error: " << job->errorString() << "for collection:" << cJob->collections();
        return;
    }

    if (!m_collectionTreeFetched && m_pendingCollectionFetchJobs.isEmpty()) {
        m_collectionTreeFetched = true;
        Q_EMIT q_ptr->collectionTreeFetched(m_collections | Views::values | Actions::toQVector);
    }

    qCDebug(DebugETM) << "Fetch job took " << jobTimeTracker.take(job).elapsed() << "msec";
    qCDebug(DebugETM) << "was collection fetch job: collections:" << cJob->collections().size();
    if (!cJob->collections().isEmpty()) {
        qCDebug(DebugETM) << "first fetched collection:" << cJob->collections().at(0).name();
    }
}

void EntityTreeModelPrivate::itemFetchJobDone(Collection::Id collectionId, KJob *job)
{
    m_pendingCollectionRetrieveJobs.remove(collectionId);

    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << "Job error: " << job->errorString() << "for collection:" << collectionId;
        return;
    }
    if (!m_collections.contains(collectionId)) {
        qCWarning(AKONADICORE_LOG) << "Collection has been removed while fetching items";
        return;
    }
    auto iJob = static_cast<ItemFetchJob *>(job);
    qCDebug(DebugETM) << "Fetch job took " << jobTimeTracker.take(job).elapsed() << "msec";
    qCDebug(DebugETM) << "was item fetch job: items:" << iJob->count();

    if (iJob->count() == 0) {
        m_collectionsWithoutItems.insert(collectionId);
    } else {
        m_collectionsWithoutItems.remove(collectionId);
    }

    m_populatedCols.insert(collectionId);
    Q_EMIT q_ptr->collectionPopulated(collectionId);

    // If collections are not in the model, there will be no valid index for them.
    if (m_collectionFetchStrategy != EntityTreeModel::FetchCollectionsMerged && //
        m_collectionFetchStrategy != EntityTreeModel::FetchNoCollections && //
        (m_showRootCollection || collectionId != m_rootCollection.id())) {
        const QModelIndex index = indexForCollection(Collection(collectionId));
        Q_ASSERT(index.isValid());
        // To notify about the changed fetch and population state
        dataChanged(index, index);
    }
}

void EntityTreeModelPrivate::pasteJobDone(KJob *job)
{
    if (job->error()) {
        QString errorMsg;
        if (qobject_cast<ItemCopyJob *>(job)) {
            errorMsg = i18nc("@info", "Could not copy item: <message>%1</message>", job->errorString());
        } else if (qobject_cast<CollectionCopyJob *>(job)) {
            errorMsg = i18nc("@info", "Could not copy collection: <message>%1</message>", job->errorString());
        } else if (qobject_cast<ItemMoveJob *>(job)) {
            errorMsg = i18nc("@info", "Could not move item: <message>%1</message>", job->errorString());
        } else if (qobject_cast<CollectionMoveJob *>(job)) {
            errorMsg = i18nc("@info", "Could not move collection: <message>%1</message>", job->errorString());
        } else if (qobject_cast<LinkJob *>(job)) {
            errorMsg = i18nc("@info", "Could not link entity: <message>%1</message>", job->errorString());
        }
        Q_EMIT q_ptr->errorOccurred(errorMsg);
    }
}

void EntityTreeModelPrivate::updateJobDone(KJob *job)
{
    if (job->error()) {
        // TODO: handle job errors
        qCWarning(AKONADICORE_LOG) << "Job error:" << job->errorString();
    }
}

void EntityTreeModelPrivate::rootFetchJobDone(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorString();
        return;
    }
    auto collectionJob = qobject_cast<CollectionFetchJob *>(job);
    const Collection::List list = collectionJob->collections();

    Q_ASSERT(list.size() == 1);
    m_rootCollection = list.first();
    startFirstListJob();
}

void EntityTreeModelPrivate::startFirstListJob()
{
    Q_Q(EntityTreeModel);

    if (!m_collections.isEmpty()) {
        return;
    }

    // Even if the root collection is the invalid collection, we still need to start
    // the first list job with Collection::root.
    auto node = new Node{Node::Collection, m_rootCollection.id(), -1};
    if (m_showRootCollection) {
        // Notify the outside that we're putting collection::root into the model.
        q->beginInsertRows(QModelIndex(), 0, 0);
        m_collections.insert(m_rootCollection.id(), m_rootCollection);
        delete m_rootNode;
        appendNode(node);
        q->endInsertRows();
    } else {
        // Otherwise store it silently because it's not part of the usable model.
        delete m_rootNode;
        m_rootNode = node;
        m_needDeleteRootNode = true;
        m_collections.insert(m_rootCollection.id(), m_rootCollection);
    }

    const bool noMimetypes = !m_mimeChecker.hasWantedMimeTypes();
    const bool noResources = m_monitor->resourcesMonitored().isEmpty();
    const bool multipleCollections = m_monitor->collectionsMonitored().size() > 1;
    const bool generalPopulation = !noMimetypes || noResources;

    const CollectionFetchJob::Type fetchType = getFetchType(m_collectionFetchStrategy);

    // Collections can only be monitored if no resources and no mimetypes are monitored
    if (multipleCollections && noMimetypes && noResources) {
        fetchCollections(m_monitor->collectionsMonitored(), CollectionFetchJob::Base);
        fetchCollections(m_monitor->collectionsMonitored(), fetchType);
        return;
    }

    qCDebug(DebugETM) << "GEN" << generalPopulation << noMimetypes << noResources;
    if (generalPopulation) {
        fetchCollections(m_rootCollection, fetchType);
    }

    // If the root collection is not collection::root, then it could have items, and they will need to be
    // retrieved now.
    // Only fetch items NOT if there is NoItemPopulation, or if there is Lazypopulation and the root is visible
    // (if the root is not visible the lazy population can not be triggered)
    if ((m_itemPopulation != EntityTreeModel::NoItemPopulation) && !((m_itemPopulation == EntityTreeModel::LazyPopulation) && m_showRootCollection)) {
        if (m_rootCollection != Collection::root()) {
            fetchItems(m_rootCollection);
        }
    }

    // Resources which are explicitly monitored won't have appeared yet if their mimetype didn't match.
    // We fetch the top level collections and examine them for whether to add them.
    // This fetches virtual collections into the tree.
    if (!m_monitor->resourcesMonitored().isEmpty()) {
        fetchTopLevelCollections();
    }
}

void EntityTreeModelPrivate::fetchTopLevelCollections() const
{
    Q_Q(const EntityTreeModel);
    auto job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel, m_session);
    q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), q, SLOT(topLevelCollectionsFetched(Akonadi::Collection::List)));
    q->connect(job, SIGNAL(result(KJob *)), q, SLOT(collectionFetchJobDone(KJob *)));
    qCDebug(DebugETM) << "EntityTreeModelPrivate::fetchTopLevelCollections";
    jobTimeTracker[job].start();
}

void EntityTreeModelPrivate::topLevelCollectionsFetched(const Akonadi::Collection::List &list)
{
    Q_Q(EntityTreeModel);
    for (const Collection &collection : list) {
        // These collections have been explicitly shown in the Monitor,
        // but hidden trumps that for now. This may change in the future if we figure out a use for it.
        if (isHidden(collection)) {
            continue;
        }

        if (m_monitor->resourcesMonitored().contains(collection.resource().toUtf8()) && !m_collections.contains(collection.id())) {
            const QModelIndex parentIndex = indexForCollection(collection.parentCollection());
            // Prepending new collections.
            const int row = 0;
            q->beginInsertRows(parentIndex, row, row);

            m_collections.insert(collection.id(), collection);
            Q_ASSERT(collection.parentCollection() == Collection::root());
            prependNode(new Node{Node::Collection, collection.id(), collection.parentCollection().id()});
            q->endInsertRows();

            if (m_itemPopulation == EntityTreeModel::ImmediatePopulation) {
                fetchItems(collection);
            }

            Q_ASSERT(collection.isValid());
            fetchCollections(collection, CollectionFetchJob::Recursive);
        }
    }
}

Akonadi::Collection::List EntityTreeModelPrivate::getParentCollections(const Item &item) const
{
    Collection::List list;
    for (auto it = m_childEntities.constKeyValueBegin(), end = m_childEntities.constKeyValueEnd(); it != end; ++it) {
        const auto &[parentId, childNodes] = *it;
        int nodeIndex = indexOf(childNodes, Node::Item, item.id());
        if (nodeIndex != -1 && childNodes.at(nodeIndex)->type == Node::Item) {
            list.push_back(m_collections.value(parentId));
        }
    }

    return list;
}

void EntityTreeModelPrivate::ref(Collection::Id id)
{
    m_monitor->d_ptr->ref(id);
}

bool EntityTreeModelPrivate::shouldPurge(Collection::Id id) const
{
    // reference counted collections should never be purged
    // they first have to be deref'ed until they reach 0.
    // if the collection is buffered, keep it, otherwise we can safely purge this item
    return !m_monitor->d_ptr->isMonitored(id);
}

bool EntityTreeModelPrivate::isMonitored(Collection::Id id) const
{
    return m_monitor->d_ptr->isMonitored(id);
}

bool EntityTreeModelPrivate::isBuffered(Collection::Id id) const
{
    return m_monitor->d_ptr->m_buffer.isBuffered(id);
}

void EntityTreeModelPrivate::deref(Collection::Id id)
{
    const Collection::Id bumpedId = m_monitor->d_ptr->deref(id);

    if (bumpedId < 0) {
        return;
    }

    // The collection has already been removed, don't purge
    if (!m_collections.contains(bumpedId)) {
        return;
    }

    if (shouldPurge(bumpedId)) {
        purgeItems(bumpedId);
    }
}

QList<Node *>::iterator EntityTreeModelPrivate::skipCollections(QList<Node *>::iterator it, const QList<Node *>::iterator &end, int *pos)
{
    for (; it != end; ++it) {
        if ((*it)->type == Node::Item) {
            break;
        }

        ++(*pos);
    }

    return it;
}

QList<Node *>::iterator
EntityTreeModelPrivate::removeItems(QList<Node *>::iterator it, const QList<Node *>::iterator &end, int *pos, const Collection &collection)
{
    Q_Q(EntityTreeModel);

    QList<Node *>::iterator startIt = it;

    // figure out how many items we will delete
    int start = *pos;
    for (; it != end; ++it) {
        if ((*it)->type != Node::Item) {
            break;
        }

        ++(*pos);
    }
    it = startIt;

    const QModelIndex parentIndex = indexForCollection(collection);

    q->beginRemoveRows(parentIndex, start, (*pos) - 1);
    const int toDelete = (*pos) - start;
    Q_ASSERT(toDelete > 0);

    QList<Node *> &es = m_childEntities[collection.id()];
    // NOTE: .erase will invalidate all iterators besides "it"!
    for (int i = 0; i < toDelete; ++i) {
        Q_ASSERT(es.count(*it) == 1);
        // don't keep implicitly shared data alive
        Q_ASSERT(m_items.contains((*it)->id));
        m_items.unref((*it)->id);
        // delete actual node
        delete *it;
        it = es.erase(it);
    }
    q->endRemoveRows();

    return it;
}

void EntityTreeModelPrivate::purgeItems(Collection::Id id)
{
    QList<Node *> &childEntities = m_childEntities[id];

    const Collection collection = m_collections.value(id);
    Q_ASSERT(collection.isValid());

    QList<Node *>::iterator begin = childEntities.begin();
    QList<Node *>::iterator end = childEntities.end();

    int pos = 0;
    while ((begin = skipCollections(begin, end, &pos)) != end) {
        begin = removeItems(begin, end, &pos, collection);
        end = childEntities.end();
    }
    m_populatedCols.remove(id);
    // if an empty collection is purged and we leave it in here, itemAdded will be ignored for the collection
    // and the collection is never populated by fetchMore (but maybe by statistics changed?)
    m_collectionsWithoutItems.remove(id);
}

void EntityTreeModelPrivate::dataChanged(const QModelIndex &top, const QModelIndex &bottom)
{
    Q_Q(EntityTreeModel);

    QModelIndex rightIndex;

    const Node *node = static_cast<Node *>(bottom.internalPointer());
    if (!node) {
        return;
    }

    if (node->type == Node::Collection) {
        rightIndex = bottom.sibling(bottom.row(), q->entityColumnCount(EntityTreeModel::CollectionTreeHeaders) - 1);
    }
    if (node->type == Node::Item) {
        rightIndex = bottom.sibling(bottom.row(), q->entityColumnCount(EntityTreeModel::ItemListHeaders) - 1);
    }

    Q_EMIT q->dataChanged(top, rightIndex);
}

QModelIndex EntityTreeModelPrivate::indexForCollection(const Collection &collection) const
{
    Q_Q(const EntityTreeModel);

    if (!collection.isValid()) {
        return QModelIndex();
    }

    if (m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsMerged) {
        return QModelIndex();
    }

    // The id of the parent of Collection::root is not guaranteed to be -1 as assumed by startFirstListJob,
    // we ensure that we use -1 for the invalid Collection.
    Collection::Id parentId = -1;

    if ((collection == m_rootCollection)) {
        if (m_showRootCollection) {
            return q->createIndex(0, 0, static_cast<void *>(m_rootNode));
        }
        return QModelIndex();
    }

    if (collection == Collection::root()) {
        parentId = -1;
    } else if (collection.parentCollection().isValid()) {
        parentId = collection.parentCollection().id();
    } else {
        for (const auto &children : m_childEntities) {
            const int row = indexOf(children, Node::Collection, collection.id());
            if (row < 0) {
                continue;
            }

            Node *node = children.at(row);
            return q->createIndex(row, 0, static_cast<void *>(node));
        }
        return QModelIndex();
    }

    const int row = indexOf(m_childEntities.value(parentId), Node::Collection, collection.id());

    if (row < 0) {
        return QModelIndex();
    }

    Node *node = m_childEntities.value(parentId).at(row);

    return q->createIndex(row, 0, static_cast<void *>(node));
}

QModelIndexList EntityTreeModelPrivate::indexesForItem(const Item &item) const
{
    Q_Q(const EntityTreeModel);
    QModelIndexList indexes;

    if (m_collectionFetchStrategy == EntityTreeModel::FetchNoCollections) {
        Q_ASSERT(m_childEntities.contains(m_rootCollection.id()));
        QList<Node *> nodeList = m_childEntities.value(m_rootCollection.id());
        const int row = indexOf(nodeList, Node::Item, item.id());
        Q_ASSERT(row >= 0);
        Q_ASSERT(row < nodeList.size());
        Node *node = nodeList.at(row);

        indexes << q->createIndex(row, 0, static_cast<void *>(node));
        return indexes;
    }

    const Collection::List collections = getParentCollections(item);

    indexes.reserve(collections.size());
    for (const Collection &collection : collections) {
        const int row = indexOf(m_childEntities.value(collection.id()), Node::Item, item.id());
        Q_ASSERT(row >= 0);
        Q_ASSERT(m_childEntities.contains(collection.id()));
        QList<Node *> nodeList = m_childEntities.value(collection.id());
        Q_ASSERT(row < nodeList.size());
        Node *node = nodeList.at(row);

        indexes << q->createIndex(row, 0, static_cast<void *>(node));
    }

    return indexes;
}

void EntityTreeModelPrivate::beginResetModel()
{
    Q_Q(EntityTreeModel);
    q->beginResetModel();
}

void EntityTreeModelPrivate::endResetModel()
{
    Q_Q(EntityTreeModel);
    const auto subjobs = m_session->findChildren<Akonadi::Job *>();
    for (auto job : subjobs) {
        job->disconnect(q);
    }
    m_collections.clear();
    m_collectionsWithoutItems.clear();
    m_populatedCols.clear();
    m_items.clear();
    m_pendingCollectionFetchJobs.clear();
    m_pendingCollectionRetrieveJobs.clear();
    m_collectionTreeFetched = false;

    for (const QList<Node *> &list : std::as_const(m_childEntities)) {
        qDeleteAll(list);
    }
    m_childEntities.clear();
    if (m_needDeleteRootNode) {
        m_needDeleteRootNode = false;
        delete m_rootNode;
    }
    m_rootNode = nullptr;

    q->endResetModel();
    fillModel();
}

void EntityTreeModelPrivate::monitoredItemsRetrieved(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorString();
        return;
    }

    Q_Q(EntityTreeModel);

    auto fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob);
    const Item::List list = fetchJob->items();

    q->beginResetModel();
    for (const Item &item : list) {
        m_childEntities[-1].append(new Node{Node::Item, item.id(), m_rootCollection.id()});
        m_items.ref(item.id(), item);
    }
    q->endResetModel();
}

void EntityTreeModelPrivate::fillModel()
{
    Q_Q(EntityTreeModel);

    m_mimeChecker.setWantedMimeTypes(m_monitor->mimeTypesMonitored());

    const Collection::List collections = m_monitor->collectionsMonitored();

    if (collections.isEmpty() && m_monitor->numMimeTypesMonitored() == 0 && m_monitor->numResourcesMonitored() == 0 && m_monitor->numItemsMonitored() != 0) {
        m_rootCollection = Collection(-1);
        m_collectionTreeFetched = true;
        Q_EMIT q_ptr->collectionTreeFetched(collections); // there are no collections to fetch

        const auto items = m_monitor->itemsMonitoredEx() | Views::transform([](const auto id) {
                               return Item{id};
                           })
            | Actions::toQVector;
        auto itemFetch = new ItemFetchJob(items, m_session);
        itemFetch->setFetchScope(m_monitor->itemFetchScope());
        itemFetch->fetchScope().setIgnoreRetrievalErrors(true);
        q->connect(itemFetch, SIGNAL(finished(KJob *)), q, SLOT(monitoredItemsRetrieved(KJob *)));
        return;
    }
    // In case there is only a single collection monitored, we can use this
    // collection as root of the node tree, in all other cases
    // Collection::root() is used
    if (collections.size() == 1) {
        m_rootCollection = collections.first();
    } else {
        m_rootCollection = Collection::root();
    }

    if (m_rootCollection == Collection::root()) {
        QTimer::singleShot(0, q, SLOT(startFirstListJob()));
    } else {
        Q_ASSERT(m_rootCollection.isValid());
        auto rootFetchJob = new CollectionFetchJob(m_rootCollection, CollectionFetchJob::Base, m_session);
        q->connect(rootFetchJob, SIGNAL(result(KJob *)), SLOT(rootFetchJobDone(KJob *)));
        qCDebug(DebugETM) << "";
        jobTimeTracker[rootFetchJob].start();
    }
}

bool EntityTreeModelPrivate::canFetchMore(const QModelIndex &parent) const
{
    const Item item = parent.data(EntityTreeModel::ItemRole).value<Item>();

    if (m_collectionFetchStrategy == EntityTreeModel::FetchCollectionsMerged) {
        return false;
    }

    if (item.isValid()) {
        // items can't have more rows.
        // TODO: Should I use this for fetching more of an item, ie more payload parts?
        return false;
    } else {
        // but collections can...
        const Collection::Id collectionId = parent.data(EntityTreeModel::CollectionIdRole).toULongLong();

        // But the root collection can't...
        if (Collection::root().id() == collectionId) {
            return false;
        }

        // Collections which contain no items at all can't contain more
        if (m_collectionsWithoutItems.contains(collectionId)) {
            return false;
        }

        // Don't start the same job multiple times.
        if (m_pendingCollectionRetrieveJobs.contains(collectionId)) {
            return false;
        }

        // Can't fetch more if the collection's items have already been fetched
        if (m_populatedCols.contains(collectionId)) {
            return false;
        }

        // Only try to fetch more from a collection if we don't already have items in it.
        // Otherwise we'd spend all the time listing items in collections.
        return m_childEntities.value(collectionId) | Actions::none(Node::isItem);
    }
}

QIcon EntityTreeModelPrivate::iconForName(const QString &name) const
{
    if (m_iconThemeName != QIcon::themeName()) {
        m_iconThemeName = QIcon::themeName();
        m_iconCache.clear();
    }

    QIcon &icon = m_iconCache[name];
    if (icon.isNull()) {
        icon = QIcon::fromTheme(name);
    }
    return icon;
}
