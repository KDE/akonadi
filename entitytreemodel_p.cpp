/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#include "entitytreemodel_p.h"
#include "entitytreemodel.h"

#include "agentmanagerinterface.h"
#include "dbusconnectionpool.h"
#include "monitor_p.h" // For friend ref/deref
#include "servermanager.h"

#include <KDE/KLocalizedString>
#include <KDE/KMessageBox>
#include <KDE/KUrl>

#include <akonadi/agentmanager.h>
#include <akonadi/agenttype.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectioncopyjob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionmovejob.h>
#include <akonadi/collectionstatistics.h>
#include <akonadi/collectionstatisticsjob.h>
#include <akonadi/entityhiddenattribute.h>
#include <akonadi/itemcopyjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/itemmovejob.h>
#include <akonadi/linkjob.h>
#include <akonadi/private/protocol_p.h>
#include <akonadi/session.h>
#include <akonadi/servermanager.h>

#include <kdebug.h>

/// comment this out to track time spent on jobs created by the ETM
// #define DBG_TRACK_JOB_TIMES

#ifdef DBG_TRACK_JOB_TIMES
QMap<KJob *, QTime> jobTimeTracker;
#define ifDebug(x) x
#else
#define ifDebug(x)
#endif

using namespace Akonadi;

static CollectionFetchJob::Type getFetchType(EntityTreeModel::CollectionFetchStrategy strategy)
{
    switch(strategy) {
        case EntityTreeModel::FetchFirstLevelChildCollections:
            return CollectionFetchJob::FirstLevel;
        case EntityTreeModel::InvisibleCollectionFetch:
        case EntityTreeModel::FetchCollectionsRecursive:
        default:
            break;
    }
    return CollectionFetchJob::Recursive;
}


EntityTreeModelPrivate::EntityTreeModelPrivate(EntityTreeModel *parent)
    : q_ptr(parent)
    , m_rootNode(0)
    , m_collectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive)
    , m_itemPopulation(EntityTreeModel::ImmediatePopulation)
    , m_listFilter(CollectionFetchScope::NoFilter)
    , m_includeStatistics(false)
    , m_showRootCollection(false)
    , m_collectionTreeFetched(false)
    , m_showSystemEntities(false)
{
    // using collection as a parameter of a queued call in runItemFetchJob()
    qRegisterMetaType<Collection>();

    org::freedesktop::Akonadi::AgentManager *manager =
        new org::freedesktop::Akonadi::AgentManager(ServerManager::serviceName(Akonadi::ServerManager::Control),
                                                    QLatin1String("/AgentManager"),
                                                    DBusConnectionPool::threadConnection(), q_ptr);

    QObject::connect(manager, SIGNAL(agentInstanceAdvancedStatusChanged(QString,QVariantMap)),
                     q_ptr, SLOT(agentInstanceAdvancedStatusChanged(QString,QVariantMap)));

    Akonadi::AgentManager *agentManager = Akonadi::AgentManager::self();
    QObject::connect(agentManager, SIGNAL(instanceRemoved(Akonadi::AgentInstance)),
                     q_ptr, SLOT(agentInstanceRemoved(Akonadi::AgentInstance)));

}

EntityTreeModelPrivate::~EntityTreeModelPrivate()
{
}

void EntityTreeModelPrivate::init(ChangeRecorder *monitor)
{
    Q_Q(EntityTreeModel);
    m_monitor = monitor;
    // The default is to FetchCollectionsRecursive, so we tell the monitor to fetch collections
    // That way update signals from the monitor will contain the full collection.
    // This may be updated if the CollectionFetchStrategy is changed.
    m_monitor->fetchCollection(true);
    m_session = m_monitor->session();

    m_monitor->setChangeRecordingEnabled(false);

    m_rootCollectionDisplayName = QLatin1String("[*]");

    m_includeStatistics = true;
    m_monitor->fetchCollectionStatistics(true);
    m_monitor->collectionFetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);

    q->connect(monitor, SIGNAL(mimeTypeMonitored(QString,bool)),
               SLOT(monitoredMimeTypeChanged(QString,bool)));
    q->connect(monitor, SIGNAL(collectionMonitored(Akonadi::Collection,bool)),
               SLOT(monitoredCollectionsChanged(Akonadi::Collection,bool)));
    q->connect(monitor, SIGNAL(itemMonitored(Akonadi::Item,bool)),
               SLOT(monitoredItemsChanged(Akonadi::Item,bool)));
    q->connect(monitor, SIGNAL(resourceMonitored(QByteArray,bool)),
               SLOT(monitoredResourcesChanged(QByteArray,bool)));

    // monitor collection changes
    q->connect(monitor, SIGNAL(collectionChanged(Akonadi::Collection)),
               SLOT(monitoredCollectionChanged(Akonadi::Collection)));
    q->connect(monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
               SLOT(monitoredCollectionAdded(Akonadi::Collection,Akonadi::Collection)));
    q->connect(monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
               SLOT(monitoredCollectionRemoved(Akonadi::Collection)));
    q->connect(monitor,
               SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)),
               SLOT(monitoredCollectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)));

    // Monitor item changes.
    q->connect(monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
               SLOT(monitoredItemAdded(Akonadi::Item,Akonadi::Collection)));
    q->connect(monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
               SLOT(monitoredItemChanged(Akonadi::Item,QSet<QByteArray>)));
    q->connect(monitor, SIGNAL(itemRemoved(Akonadi::Item)),
               SLOT(monitoredItemRemoved(Akonadi::Item)));
    q->connect(monitor, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)),
               SLOT(monitoredItemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)));

    q->connect(monitor, SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)),
               SLOT(monitoredItemLinked(Akonadi::Item,Akonadi::Collection)));
    q->connect(monitor, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)),
               SLOT(monitoredItemUnlinked(Akonadi::Item,Akonadi::Collection)));

    q->connect(monitor, SIGNAL(collectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)),
               SLOT(monitoredCollectionStatisticsChanged(Akonadi::Collection::Id,Akonadi::CollectionStatistics)));

    Akonadi::ServerManager *serverManager = Akonadi::ServerManager::self();
    q->connect(serverManager, SIGNAL(started()), SLOT(serverStarted()));

    QHash<int, QByteArray> names = q->roleNames();

    names.insert(EntityTreeModel::UnreadCountRole, "unreadCount");
    names.insert(EntityTreeModel::FetchStateRole, "fetchState");
    names.insert(EntityTreeModel::CollectionSyncProgressRole, "collectionSyncProgress");
    names.insert(EntityTreeModel::ItemIdRole, "itemId");

    q->setRoleNames(names);

    fillModel();
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
    q->dataChanged(collectionIndex, collectionIndex);
}

void EntityTreeModelPrivate::agentInstanceRemoved(const Akonadi::AgentInstance &instance)
{
    Q_Q(EntityTreeModel);
    if (!instance.type().capabilities().contains(QLatin1String("Resource"))) {
        return;
    }

    if (m_rootCollection.isValid()) {
        if (m_rootCollection != Collection::root()) {
            if (m_rootCollection.resource() == instance.identifier()) {
                q->clearAndReset();
            }
            return;
        }
        foreach (Node *node, m_childEntities[Collection::root().id()]) {
            Q_ASSERT(node->type == Node::Collection);

            const Collection collection = m_collections[node->id];
            if (collection.resource() == instance.identifier()) {
                monitoredCollectionRemoved(collection);
            }
        }
    }
}

void EntityTreeModelPrivate::agentInstanceAdvancedStatusChanged(const QString &, const QVariantMap &status)
{
    const QString key = status.value(QLatin1String("key")).toString();
    if (key != QLatin1String("collectionSyncProgress")) {
        return;
    }

    const Collection::Id collectionId = status.value(QLatin1String("collectionId")).toLongLong();
    const uint percent = status.value(QLatin1String("percent")).toUInt();
    if (m_collectionSyncProgress.value(collectionId) == percent) {
        return;
    }
    m_collectionSyncProgress.insert(collectionId, percent);

    const QModelIndex collectionIndex = indexForCollection(Collection(collectionId));
    if (!collectionIndex.isValid()) {
        return;
    }

    Q_Q(EntityTreeModel);
    // This is really slow (80 levels of method calls in proxy models...), and called
    // very often during an imap sync...
    q->dataChanged(collectionIndex, collectionIndex);
}

void EntityTreeModelPrivate::fetchItems(const Collection &parent)
{
    Q_Q(const EntityTreeModel);
    Q_ASSERT(parent.isValid());
    Q_ASSERT(m_collections.contains(parent.id()));
    // TODO: Use a more specific fetch scope to get only the envelope for mails etc.
    ItemFetchJob *itemFetchJob = new Akonadi::ItemFetchJob(parent, m_session);
    itemFetchJob->setFetchScope(m_monitor->itemFetchScope());
    itemFetchJob->fetchScope().setAncestorRetrieval(ItemFetchScope::All);
    itemFetchJob->fetchScope().setIgnoreRetrievalErrors(true);
    itemFetchJob->setDeliveryOption(ItemFetchJob::EmitItemsInBatches);

    itemFetchJob->setProperty(FetchCollectionId(), QVariant(parent.id()));

    if (m_showRootCollection || parent != m_rootCollection) {
        m_pendingCollectionRetrieveJobs.insert(parent.id());

        // If collections are not in the model, there will be no valid index for them.
        if ((m_collectionFetchStrategy != EntityTreeModel::InvisibleCollectionFetch) &&
            (m_collectionFetchStrategy != EntityTreeModel::FetchNoCollections)) {
            // We need to invoke this delayed because we would otherwise be emitting a sequence like
            // - beginInsertRows
            // - dataChanged
            // - endInsertRows
            // which would confuse proxies.
            QMetaObject::invokeMethod(const_cast<EntityTreeModel *>(q), "changeFetchState", Qt::QueuedConnection, Q_ARG(Akonadi::Collection, parent));
        }
    }

    q->connect(itemFetchJob, SIGNAL(itemsReceived(Akonadi::Item::List)),
               q, SLOT(itemsFetched(Akonadi::Item::List)));
    q->connect(itemFetchJob, SIGNAL(result(KJob*)),
               q, SLOT(itemFetchJobDone(KJob*)));
    ifDebug(kDebug() << "collection:" << parent.name(); jobTimeTracker[itemFetchJob].start();)
}

void EntityTreeModelPrivate::fetchCollections(Akonadi::CollectionFetchJob *job)
{
    Q_Q(EntityTreeModel);

    job->fetchScope().setListFilter(m_listFilter);
    job->fetchScope().setContentMimeTypes(m_monitor->mimeTypesMonitored());
    m_pendingCollectionFetchJobs.insert(static_cast<KJob*>(job));

    if (m_collectionFetchStrategy == EntityTreeModel::InvisibleCollectionFetch) {
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                   q, SLOT(collectionListFetched(Akonadi::Collection::List)));
    } else {
        job->fetchScope().setIncludeStatistics(m_includeStatistics);
        job->fetchScope().setAncestorRetrieval(Akonadi::CollectionFetchScope::All);
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                    q, SLOT(collectionsFetched(Akonadi::Collection::List)));
    }
    q->connect(job, SIGNAL(result(KJob*)),
                q, SLOT(collectionFetchJobDone(KJob*)));
    ifDebug(kDebug() << "collection:" << collection.name(); jobTimeTracker[job].start();)
}

void EntityTreeModelPrivate::fetchCollections(const Collection::List &collections, CollectionFetchJob::Type type)
{
    fetchCollections(new CollectionFetchJob(collections, type, m_session));
}

void EntityTreeModelPrivate::fetchCollections(const Collection &collection, CollectionFetchJob::Type type)
{
    Q_ASSERT(collection.isValid());
    CollectionFetchJob *job = new CollectionFetchJob(collection, type, m_session);
    fetchCollections(job);
}

// Specialization needs to be in the same namespace as the definition
namespace Akonadi {

template<>
bool EntityTreeModelPrivate::isHidden<Akonadi::Collection>(const Akonadi::Collection &entity) const
{
    return isHidden(entity, Node::Collection);
}

template<>
bool EntityTreeModelPrivate::isHidden<Akonadi::Item>(const Akonadi::Item &entity) const
{
    return isHidden(entity, Node::Item);
}

}

bool EntityTreeModelPrivate::isHidden(const Entity &entity, Node::Type type) const
{
    if (m_showSystemEntities) {
        return false;
    }

    if (type == Node::Collection &&
        entity.id() == m_rootCollection.id()) {
        return false;
    }

    if (entity.hasAttribute<EntityHiddenAttribute>()) {
        return true;
    }

    const Collection parent = entity.parentCollection();
    if (parent.isValid()) {
        return isHidden(parent, Node::Collection);
    }

    return false;
}

void EntityTreeModelPrivate::collectionListFetched(const Akonadi::Collection::List &collections)
{
    QListIterator<Akonadi::Collection> it(collections);

    while (it.hasNext()) {
        const Collection collection = it.next();

        if (isHidden(collection)) {
            continue;
        }

        m_collections.insert(collection.id(), collection);

        Node *node = new Node;
        node->id = collection.id();
        node->parent = -1;
        node->type = Node::Collection;
        m_childEntities[-1].prepend(node);

        fetchItems(collection);
    }
}

static QSet<Collection::Id> getChildren(Collection::Id parent, const QHash<Collection::Id, Collection::Id> &childParentMap)
{
    QSet<Collection::Id> children;
    Q_FOREACH (Collection::Id c, childParentMap.keys()) {
        if (childParentMap.value(c) == parent) {
            children << c;
            children += getChildren(c, childParentMap);
        }
    }
    return children;
}

void EntityTreeModelPrivate::collectionsFetched(const Akonadi::Collection::List &collections)
{
    Q_Q(EntityTreeModel);
    QTime t;
    t.start();

    QListIterator<Akonadi::Collection> it(collections);

    QHash<Collection::Id, Collection> collectionsToInsert;

    while (it.hasNext()) {
        const Collection collection = it.next();

        const Collection::Id collectionId = collection.id();

        if (isHidden(collection)) {
            continue;
        }

        if (m_collections.contains(collectionId)) {
            // This is probably the result of a parent of a previous collection already being in the model.
            // Replace the dummy collection with the real one and move on.

            // This could also be the result of a monitor signal having already inserted the collection
            // into this model. There's no way to tell, so we just emit dataChanged.

            m_collections[collectionId] = collection;

            const QModelIndex collectionIndex = indexForCollection(collection);
            dataChanged(collectionIndex, collectionIndex);
            emit q->collectionFetched(collectionId);
            continue;
        }

        //If we're monitoring collections somewhere in the tree we need to retrieve their ancestors now
        if (collection.parentCollection() != m_rootCollection && m_monitor->collectionsMonitored().contains(collection)) {
            retrieveAncestors(collection, false);
        }

        collectionsToInsert.insert(collectionId, collection);
    }

    //Build a list of subtrees to insert, with the root of the subtree on the left, and the complete subtree including root on the right
    QHash<Collection::Id, QSet<Collection::Id> > subTreesToInsert;
    {
        //Build a child-parent map that allows us to build the subtrees afterwards
        QHash<Collection::Id, Collection::Id> childParentMap;
        Q_FOREACH (const Collection &col, collectionsToInsert) {
            childParentMap.insert(col.id(), col.parentCollection().id());

            //Complete the subtree up to the last known parent
            Collection parent = col.parentCollection();
            while (parent.isValid() && parent != m_rootCollection && !m_collections.contains(parent.id())) {
                childParentMap.insert(parent.id(), parent.parentCollection().id());

                if (!collectionsToInsert.contains(parent.id())) {
                    collectionsToInsert.insert(parent.id(), parent);
                }
                parent = parent.parentCollection();
            }
        }

        QSet<Collection::Id> parents;

        //Find toplevel parents of the subtrees
        Q_FOREACH (Collection::Id c, childParentMap.keys()) {
            //The child has a parent without parent (it's a toplevel node that is not yet in m_collections)
            if (!childParentMap.contains(childParentMap.value(c))) {
                Q_ASSERT(!m_collections.contains(c));
                parents << c;
            }
        }

        //Find children of each subtree
        Q_FOREACH (Collection::Id p, parents) {
            QSet<Collection::Id> children;
            //We add the parent itself as well so it can be inserted below as part of the same loop
            children << p;
            children += getChildren(p, childParentMap);
            subTreesToInsert[p] = children;
        }
    }

    const int row = 0;

    QHashIterator<Collection::Id, QSet<Collection::Id> > collectionIt(subTreesToInsert);
    while (collectionIt.hasNext()) {
        collectionIt.next();

        const Collection::Id topCollectionId = collectionIt.key();

        Q_ASSERT(!m_collections.contains(topCollectionId));
        Collection topCollection = collectionsToInsert.value(topCollectionId);
        Q_ASSERT(topCollection.isValid());

        //The toplevels parent must already be part of the model
        Q_ASSERT(m_collections.contains(topCollection.parentCollection().id()));
        const QModelIndex parentIndex = indexForCollection(topCollection.parentCollection());

        q->beginInsertRows(parentIndex, row, row);
        Q_ASSERT(!collectionIt.value().isEmpty());

        foreach (Collection::Id collectionId, collectionIt.value()) {
            const Collection collection = collectionsToInsert.take(collectionId);
            Q_ASSERT(collection.isValid());

            m_collections.insert(collectionId, collection);

            Node *node = new Node;
            node->id = collectionId;
            Q_ASSERT(collection.parentCollection().isValid());
            node->parent = collection.parentCollection().id();
            node->type = Node::Collection;
            m_childEntities[node->parent].prepend(node);
        }
        q->endInsertRows();

        if (m_itemPopulation == EntityTreeModel::ImmediatePopulation) {
            foreach (const Collection::Id &collectionId, collectionIt.value()) {
                fetchItems(m_collections.value(collectionId));
            }
        }
    }
}

void EntityTreeModelPrivate::itemsFetched(const Akonadi::Item::List &items)
{
    Q_Q(EntityTreeModel);

    const Collection::Id collectionId = q->sender()->property(FetchCollectionId()).value<Collection::Id>();

    itemsFetched(collectionId, items);
}

void EntityTreeModelPrivate::itemsFetched(const Collection::Id collectionId, const Akonadi::Item::List &items)
{
    Q_Q(EntityTreeModel);

    if (!m_collections.contains(collectionId)) {
        kWarning() << "Collection has been removed while fetching items";
        return;
    }

    Item::List itemsToInsert;

    const Collection collection = m_collections.value(collectionId);

    Q_ASSERT(collection.isValid());

    // if there are any items at all, remove from set of collections known to be empty
    if (!items.isEmpty()) {
        m_collectionsWithoutItems.remove(collectionId);
    }

    foreach (const Item &item, items) {

        if (isHidden(item)) {
            continue;
        }

        if ((m_mimeChecker.wantedMimeTypes().isEmpty() ||
             m_mimeChecker.isWantedItem(item))) {
            // When listing virtual collections we might get results for items which are already in
            // the model if their concrete collection has already been listed.
            // In that case the collectionId should be different though.

            // As an additional complication, new items might be both part of fetch job results and
            // part of monitor notifications. We only insert items which are not already in the model
            // considering their (possibly virtual) parent.
            bool isNewItem = true;
            if (m_items.contains(item.id())) {
                const Akonadi::Collection::List parents = getParentCollections(item);
                foreach (const Akonadi::Collection &parent, parents) {
                    if (parent.id() == collectionId) {
                        kWarning() << "Fetched an item which is already in the model";
                        // Update it in case the revision changed;
                        m_items[item.id()].apply(item);
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

    if (itemsToInsert.size() > 0) {
        const Collection::Id colId = m_collectionFetchStrategy == EntityTreeModel::InvisibleCollectionFetch ? m_rootCollection.id()
                                   : m_collectionFetchStrategy == EntityTreeModel::FetchNoCollections ? m_rootCollection.id()
                                   : collectionId;
        const int startRow = m_childEntities.value(colId).size();

        Q_ASSERT(m_collections.contains(colId));

        const QModelIndex parentIndex = indexForCollection(m_collections.value(colId));
        q->beginInsertRows(parentIndex, startRow, startRow + itemsToInsert.size() - 1);

        foreach (const Item &item, itemsToInsert) {
            const Item::Id itemId = item.id();
            // Don't reinsert when listing virtual collections.
            if (!m_items.contains(item.id())) {
                m_items.insert(itemId, item);
            }

            Node *node = new Node;
            node->id = itemId;
            node->parent = collectionId;
            node->type = Node::Item;

            m_childEntities[colId].append(node);
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
        //If a collection is derefernced and no longer explicitly monitored it might still match other filters
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

void EntityTreeModelPrivate::retrieveAncestors(const Akonadi::Collection &collection, bool insertBaseCollection)
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
    }
    Q_ASSERT(parentCollection.isValid());
    // if m_rootCollection is Collection::root(), we always have common ancestor and do the retrival
    // if we traversed up to Collection::root() but are looking at a subtree only (m_rootCollection != Collection::root())
    // we have no common ancestor, and we don't have to retrieve anything
    if (parentCollection == Collection::root() && m_rootCollection != Collection::root()) {
        return;
    }

    if (ancestors.isEmpty() && !insertBaseCollection) {
        //Nothing to do, avoid emitting insert signals
        return;
    }

    if (!ancestors.isEmpty()) {
        // Fetch the real ancestors
        CollectionFetchJob *job = new CollectionFetchJob(ancestors, CollectionFetchJob::Base, m_session);
        job->fetchScope().setListFilter(m_listFilter);
        job->fetchScope().setIncludeStatistics(m_includeStatistics);
        q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                   q, SLOT(ancestorsFetched(Akonadi::Collection::List)));
        q->connect(job, SIGNAL(result(KJob*)),
                   q, SLOT(collectionFetchJobDone(KJob*)));
    }

//  Q_ASSERT( parentCollection != m_rootCollection );
    const QModelIndex parent = indexForCollection(parentCollection);

    // Still prepending all collections for now.
    int row = 0;

    // Although we insert several Collections here, we only need to notify though the model
    // about the top-level one. The rest will be found auotmatically by the view.
    q->beginInsertRows(parent, row, row);


    Collection::List::const_iterator it = ancestors.constBegin();
    const Collection::List::const_iterator end = ancestors.constEnd();

    for (; it != end; ++it) {
        const Collection ancestor = *it;
        Q_ASSERT(ancestor.parentCollection().isValid());
        m_collections.insert(ancestor.id(), ancestor);

        Node *node = new Node;
        node->id = ancestor.id();
        node->parent = ancestor.parentCollection().id();
        node->type = Node::Collection;
        m_childEntities[node->parent].prepend(node);
    }

    if (insertBaseCollection) {
        m_collections.insert(collection.id(), collection);
        Node *node = new Node;
        node->id = collection.id();
        // Can't just use parentCollection because that doesn't necessarily refer to collection.
        node->parent = collection.parentCollection().id();
        node->type = Node::Collection;
        m_childEntities[node->parent].prepend(node);
    }

    q->endInsertRows();
}

void EntityTreeModelPrivate::ancestorsFetched(const Akonadi::Collection::List &collectionList)
{
    foreach (const Collection &collection, collectionList) {
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

    Node *node = new Node;
    node->id = collection.id();
    node->parent = parent.id();
    node->type = Node::Collection;
    m_childEntities[parent.id()].prepend(node);
    q->endInsertRows();
}

bool EntityTreeModelPrivate::hasChildCollection(const Collection &collection) const
{
    foreach (Node *node, m_childEntities[collection.id()]) {
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

    //Explicitly monitored collection
    if (m_monitor->collectionsMonitored().contains(collection)) {
        return true;
    }

    //We're explicitly monitoring collections, but didn't match the filter
    if (m_mimeChecker.wantedMimeTypes().isEmpty() && !m_monitor->collectionsMonitored().isEmpty()) {
        //The collection should be included if one of the parents is monitored
        if (isAncestorMonitored(collection)) {
            return true;
        }
        return false;
    }

    // Some collection trees contain multiple mimetypes. Even though server side filtering ensures we
    // only get the ones we're interested in from the job, we have to filter on collections received through signals too.
    if (!m_mimeChecker.wantedMimeTypes().isEmpty() &&
        !m_mimeChecker.isWantedCollection(collection)) {
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

    //If the resource is explicitly monitored all other checks are skipped. topLevelCollectionsFetched still checks the hidden attribute.
    if (m_monitor->resourcesMonitored().contains(collection.resource().toUtf8()) &&
        collection.parentCollection() == Collection::root()) {
        return topLevelCollectionsFetched(Collection::List() << collection);
    }

    if (!shouldBePartOfModel(collection)) {
        return;
    }

    if (!m_collections.contains(parent.id())) {
        // The collection we're interested in is contained in a collection we're not interested in.
        // We download the ancestors of the collection we're interested in to complete the tree.
        if (collection != Collection::root()) {
            retrieveAncestors(collection);
        }
        if (m_itemPopulation == EntityTreeModel::ImmediatePopulation) {
            fetchItems(collection);
        }
        return;
    }

    insertCollection(collection, parent);
    if (m_itemPopulation == EntityTreeModel::ImmediatePopulation) {
        fetchItems(collection);
    }
}

void EntityTreeModelPrivate::monitoredCollectionRemoved(const Akonadi::Collection &collection)
{
    //if an explictly monitored collection is removed, we would also have to remove collections which were included to show it (as in the move case)
    if ((collection == m_rootCollection) ||
        m_monitor->collectionsMonitored().contains(collection)) {
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

    const int row = indexOf<Node::Collection>(m_childEntities.value(parentId), collection.id());

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

void EntityTreeModelPrivate::removeChildEntities(Collection::Id collectionId)
{
    QList<Node *> childList = m_childEntities.value(collectionId);
    QList<Node *>::const_iterator it = childList.constBegin();
    const QList<Node *>::const_iterator end = childList.constEnd();
    for (; it != end; ++it) {
        if (Node::Item == (*it)->type) {
            m_items.remove((*it)->id);
        } else {
            removeChildEntities((*it)->id);
            m_collections.remove((*it)->id);
            m_populatedCols.remove((*it)->id);
        }
    }

    qDeleteAll(m_childEntities.take(collectionId));
}

QStringList EntityTreeModelPrivate::childCollectionNames(const Collection &collection) const
{
    QStringList names;

    foreach (Node *node, m_childEntities[collection.id()]) {
        if (node->type == Node::Collection) {
            names << m_collections.value(node->id).name();
        }
    }

    return names;
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
        //if we don't reset here, we would have to make sure that destination collection is actually available,
        //and remove the sources parents if they were only included as parents of the moved collection
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

    const int srcRow = indexOf<Node::Collection>(m_childEntities.value(sourceCollection.id()), collection.id());
    const int destRow = 0; // Prepend collections

    if (!q->beginMoveRows(srcParentIndex, srcRow, srcRow, destParentIndex, destRow)) {
        kWarning() << "Invalid move";
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

        //We might match the filter now, retry adding the collection
        monitoredCollectionAdded(collection, collection.parentCollection());
        return;
    }

    if (!shouldBePartOfModel(collection)) {
        monitoredCollectionRemoved(collection);
        return;
    }

    m_collections[collection.id()] = collection;

    if (!m_showRootCollection &&
        collection == m_rootCollection) {
        // If the root of the model is not Collection::root it might be modified.
        // But it doesn't exist in the accessible model structure, so we need to early return
        return;
    }

    const QModelIndex index = indexForCollection(collection);
    Q_ASSERT(index.isValid());
    dataChanged(index, index);
}

void EntityTreeModelPrivate::monitoredCollectionStatisticsChanged(Akonadi::Collection::Id id,
                                                                  const Akonadi::CollectionStatistics &statistics)
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

    if (!m_showRootCollection &&
        id == m_rootCollection.id()) {
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

    if (m_collectionFetchStrategy != EntityTreeModel::InvisibleCollectionFetch &&
        !m_collections.contains(collection.id())) {
        kWarning() << "Got a stale notification for an item whose collection was already removed." << item.id() << item.remoteId();
        return;
    }

    if (m_items.contains(item.id())) {
        return;
    }

    Q_ASSERT(m_collectionFetchStrategy != EntityTreeModel::InvisibleCollectionFetch ? m_collections.contains(collection.id()) : true);

    if (!m_mimeChecker.wantedMimeTypes().isEmpty() &&
        !m_mimeChecker.isWantedItem(item)) {
        return;
    }

    //Adding items to not yet populated collections would block fetchMore, resulting in only new items showing up in the collection
    //This is only a problem with lazy population, otherwise fetchMore is not used at all
    if ((m_itemPopulation == EntityTreeModel::LazyPopulation) && !m_populatedCols.contains(collection.id())) {
        return;
    }

    int row;
    QModelIndex parentIndex;
    if (m_collectionFetchStrategy != EntityTreeModel::InvisibleCollectionFetch) {
        row = m_childEntities.value(collection.id()).size();
        parentIndex = indexForCollection(m_collections.value(collection.id()));
    } else {
        row = q->rowCount();
    }
    q->beginInsertRows(parentIndex, row, row);
    m_items.insert(item.id(), item);
    Node *node = new Node;
    node->id = item.id();
    node->parent = collection.id();
    node->type = Node::Item;
    if (m_collectionFetchStrategy != EntityTreeModel::InvisibleCollectionFetch) {
        m_childEntities[collection.id()].append(node);
    } else {
        m_childEntities[m_rootCollection.id()].append(node);
    }
    q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemRemoved(const Akonadi::Item &item)
{
    Q_Q(EntityTreeModel);

    if (isHidden(item)) {
        return;
    }

    const Collection::List parents = getParentCollections(item);
    if (parents.isEmpty()) {
        return;
    }

    if (!m_items.contains(item.id())) {
        kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    // TODO: Iterate over all (virtual) collections.
    const Collection collection = parents.first();

    Q_ASSERT(m_collections.contains(collection.id()));
    Q_ASSERT(m_childEntities.contains(collection.id()));

    const int row = indexOf<Node::Item>(m_childEntities.value(collection.id()), item.id());
    Q_ASSERT(row >= 0);

    const QModelIndex parentIndex = indexForCollection(m_collections.value(collection.id()));

    q->beginRemoveRows(parentIndex, row, row);
    m_items.remove(item.id());
    delete m_childEntities[collection.id()].takeAt(row);
    q->endRemoveRows();
}

void EntityTreeModelPrivate::monitoredItemChanged(const Akonadi::Item &item, const QSet<QByteArray> &)
{
    if (isHidden(item)) {
        return;
    }

    if (!m_items.contains(item.id())) {
        kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    // Notifications about itemChange are always dispatched for real collection
    // and also all virtual collections the item belongs to. In order to preserve
    // the original storage collection when we need to have special handling for
    // notifications for virtual collections
    if (item.parentCollection().isVirtual()) {
        const Collection originalParent = m_items[item.id()].parentCollection();
        m_items[item.id()].apply(item);
        m_items[item.id()].setParentCollection(originalParent);
    } else {
        m_items[item.id()].apply(item);
    }

    const QModelIndexList indexes = indexesForItem(item);
    foreach (const QModelIndex &index, indexes) {
        if (!index.isValid()) {
            kWarning() << "item has invalid index:" << item.id() << item.remoteId();
        } else {
            dataChanged(index, index);
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
        monitoredItemRemoved(item);
        return;
    } else {
        monitoredItemRemoved(item);
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
        kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    Q_ASSERT(m_collections.contains(sourceCollection.id()));
    Q_ASSERT(m_collections.contains(destCollection.id()));

    const QModelIndex srcIndex = indexForCollection(sourceCollection);
    const QModelIndex destIndex = indexForCollection(destCollection);

    // Where should it go? Always append items and prepend collections and reorganize them with separate reactions to Attributes?

    const Item::Id itemId = item.id();

    const int srcRow = indexOf<Node::Item>(m_childEntities.value(sourceCollection.id()), itemId);
    const int destRow = q->rowCount(destIndex);

    Q_ASSERT(srcRow >= 0);
    Q_ASSERT(destRow >= 0);
    if (!q->beginMoveRows(srcIndex, srcRow, srcRow, destIndex, destRow)) {
        kWarning() << "Invalid move";
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

    Q_ASSERT(m_collections.contains(collectionId));

    if (!m_mimeChecker.wantedMimeTypes().isEmpty() &&
        !m_mimeChecker.isWantedItem(item)) {
        return;
    }

    QList<Node *> &collectionEntities =  m_childEntities[collectionId];

    int existingPosition = indexOf<Node::Item>(collectionEntities, itemId);

    if (existingPosition > 0) {
        qWarning() << "Item with id " << itemId << " already in virtual collection with id " << collectionId;
        return;
    }

    const int row = collectionEntities.size();

    const QModelIndex parentIndex = indexForCollection(m_collections.value(collectionId));

    q->beginInsertRows(parentIndex, row, row);
    if (!m_items.contains(itemId)) {
        m_items.insert(itemId, item);
    }
    Node *node = new Node;
    node->id = itemId;
    node->parent = collectionId;
    node->type = Node::Item;
    collectionEntities.append(node);
    q->endInsertRows();
}

void EntityTreeModelPrivate::monitoredItemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_Q(EntityTreeModel);

    if (isHidden(item)) {
        return;
    }

    if (!m_items.contains(item.id())) {
        kWarning() << "Got a stale notification for an item which was already removed." << item.id() << item.remoteId();
        return;
    }

    Q_ASSERT(m_collections.contains(collection.id()));

    const int row = indexOf<Node::Item>( m_childEntities.value( collection.id() ), item.id() );
    if ( row < 0 || row >= m_childEntities[ collection.id() ].size() ) {
       kWarning() << "couldn't find index of unlinked item " << item.id() << collection.id() << row;
       Q_ASSERT(false);
       return;
    }

    const QModelIndex parentIndex = indexForCollection(m_collections.value(collection.id()));

    q->beginRemoveRows(parentIndex, row, row);
    delete m_childEntities[collection.id()].takeAt(row);
    q->endRemoveRows();
}

void EntityTreeModelPrivate::collectionFetchJobDone(KJob *job)
{
    m_pendingCollectionFetchJobs.remove(job);
    CollectionFetchJob *cJob = static_cast<CollectionFetchJob *>(job);
    if (job->error()) {
        kWarning() << "Job error: " << job->errorString() << "for collection:" << cJob->collections() << endl;
        return;
    }

    if (!m_collectionTreeFetched && m_pendingCollectionFetchJobs.isEmpty()) {
        m_collectionTreeFetched = true;
        emit q_ptr->collectionTreeFetched(m_collections.values());
    }

#ifdef DBG_TRACK_JOB_TIMES
    kDebug() << "Fetch job took " << jobTimeTracker.take(job).elapsed() << "msec";
    kDebug() << "was collection fetch job: collections:" << cJob->collections().size();
    if (!cJob->collections().isEmpty()) {
        kDebug() << "first fetched collection:" << cJob->collections().first().name();
    }
#endif
}

void EntityTreeModelPrivate::itemFetchJobDone(KJob *job)
{
    const Collection::Id collectionId = job->property(FetchCollectionId()).value<Collection::Id>();
    m_pendingCollectionRetrieveJobs.remove(collectionId);

    if (job->error()) {
        kWarning() << "Job error: " << job->errorString() << "for collection:" << collectionId << endl;
        return;
    }
    if (!m_collections.contains(collectionId)) {
        kWarning() << "Collection has been removed while fetching items";
        return;
    }
    ItemFetchJob *iJob = static_cast<ItemFetchJob *>(job);

#ifdef DBG_TRACK_JOB_TIMES
    kDebug() << "Fetch job took " << jobTimeTracker.take(job).elapsed() << "msec";
    kDebug() << "was item fetch job: items:" << iJob->items().size();
    if (!iJob->items().isEmpty()) {
        kDebug() << "first item collection:" << iJob->items().first().parentCollection().name();
    }
#endif

    if (!iJob->count()) {
        m_collectionsWithoutItems.insert(collectionId);
    } else {
        m_collectionsWithoutItems.remove(collectionId);
    }

    m_populatedCols.insert(collectionId);
    emit q_ptr->collectionPopulated(collectionId);

    // If collections are not in the model, there will be no valid index for them.
    if ((m_collectionFetchStrategy != EntityTreeModel::InvisibleCollectionFetch) &&
        (m_collectionFetchStrategy != EntityTreeModel::FetchNoCollections) &&
        !(!m_showRootCollection && collectionId == m_rootCollection.id())) {
        const QModelIndex index = indexForCollection(Collection(collectionId));
        Q_ASSERT(index.isValid());
        //To notify about the changed fetch and population state
        emit dataChanged(index, index);
    }
}

void EntityTreeModelPrivate::pasteJobDone(KJob *job)
{
    if (job->error()) {
        QString errorMsg;
        if (qobject_cast<ItemCopyJob *>(job)) {
            errorMsg = i18n("Could not copy item:");
        } else if (qobject_cast<CollectionCopyJob *>(job)) {
            errorMsg = i18n("Could not copy collection:");
        } else if (qobject_cast<ItemMoveJob *>(job)) {
            errorMsg = i18n("Could not move item:");
        } else if (qobject_cast<CollectionMoveJob *>(job)) {
            errorMsg = i18n("Could not move collection:");
        } else if (qobject_cast<LinkJob *>(job)) {
            errorMsg = i18n("Could not link entity:");
        }

        errorMsg += QLatin1Char(' ') + job->errorString();

        KMessageBox::error(0, errorMsg);
    }
}

void EntityTreeModelPrivate::updateJobDone(KJob *job)
{
    if (job->error()) {
        // TODO: handle job errors
        kWarning() << "Job error:" << job->errorString();
    } else {

        //FIXME: This seems pretty pointless since we'll get an update through the monitor anyways
        ItemModifyJob *modifyJob = qobject_cast<ItemModifyJob *>(job);
        if (!modifyJob) {
            return;
        }

        const Item item = modifyJob->item();

        Q_ASSERT(item.isValid());

        m_items[item.id()].apply(item);
        const QModelIndexList list = indexesForItem(item);

        foreach (const QModelIndex &index, list) {
            dataChanged(index, index);
        }
    }
}

void EntityTreeModelPrivate::rootFetchJobDone(KJob *job)
{
    if (job->error()) {
        kWarning() << job->errorString();
        return;
    }
    CollectionFetchJob *collectionJob = qobject_cast<CollectionFetchJob *>(job);
    const Collection::List list = collectionJob->collections();

    Q_ASSERT(list.size() == 1);
    m_rootCollection = list.first();
    startFirstListJob();
}

void EntityTreeModelPrivate::startFirstListJob()
{
    Q_Q(EntityTreeModel);

    if (m_collections.size() > 0) {
        return;
    }

    // Even if the root collection is the invalid collection, we still need to start
    // the first list job with Collection::root.
    if (m_showRootCollection) {
        // Notify the outside that we're putting collection::root into the model.
        q->beginInsertRows(QModelIndex(), 0, 0);
        m_collections.insert(m_rootCollection.id(), m_rootCollection);
        delete m_rootNode;
        m_rootNode = new Node;
        m_rootNode->id = m_rootCollection.id();
        m_rootNode->parent = -1;
        m_rootNode->type = Node::Collection;
        m_childEntities[-1].append(m_rootNode);
        q->endInsertRows();
    } else {
        // Otherwise store it silently because it's not part of the usable model.
        delete m_rootNode;
        m_rootNode = new Node;
        m_rootNode->id = m_rootCollection.id();
        m_rootNode->parent = -1;
        m_rootNode->type = Node::Collection;
        m_collections.insert(m_rootCollection.id(), m_rootCollection);
    }

    const bool noMimetypes = m_mimeChecker.wantedMimeTypes().isEmpty();
    const bool noResources = m_monitor->resourcesMonitored().isEmpty();
    const bool multipleCollections = m_monitor->collectionsMonitored().size() > 1;
    const bool generalPopulation = !noMimetypes || (noMimetypes && noResources);

    const CollectionFetchJob::Type fetchType = getFetchType(m_collectionFetchStrategy);

    //Collections can only be monitored if no resources and no mimetypes are monitored
    if (multipleCollections && noMimetypes && noResources) {
        fetchCollections(m_monitor->collectionsMonitored(), CollectionFetchJob::Base);
        fetchCollections(m_monitor->collectionsMonitored(), fetchType);
        return;
    }

    kDebug() << "GEN" << generalPopulation << noMimetypes << noResources;
    if (generalPopulation) {
        fetchCollections(m_rootCollection, fetchType);
    }

    // If the root collection is not collection::root, then it could have items, and they will need to be
    // retrieved now.
    // Only fetch items NOT if there is NoItemPopulation, or if there is Lazypopulation and the root is visible
    // (if the root is not visible the lazy population can not be triggered)
    if ((m_itemPopulation != EntityTreeModel::NoItemPopulation) &&
        !((m_itemPopulation == EntityTreeModel::LazyPopulation) &&
          m_showRootCollection)) {
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
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel, m_session);
    q->connect(job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
               q, SLOT(topLevelCollectionsFetched(Akonadi::Collection::List)));
    q->connect(job, SIGNAL(result(KJob*)),
               q, SLOT(collectionFetchJobDone(KJob*)));
    ifDebug(kDebug() << ""; jobTimeTracker[job].start();)
}

void EntityTreeModelPrivate::topLevelCollectionsFetched(const Akonadi::Collection::List &list)
{
    Q_Q(EntityTreeModel);
    foreach (const Collection &collection, list) {
        // These collections have been explicitly shown in the Monitor,
        // but hidden trumps that for now. This may change in the future if we figure out a use for it.
        if (isHidden(collection)) {
            continue;
        }

        if (m_monitor->resourcesMonitored().contains(collection.resource().toUtf8()) &&
            !m_collections.contains(collection.id())) {
            const QModelIndex parentIndex = indexForCollection(collection.parentCollection());
            // Prepending new collections.
            const int row  = 0;
            q->beginInsertRows(parentIndex, row, row);

            m_collections.insert(collection.id(), collection);
            Node *node = new Node;
            node->id = collection.id();
            Q_ASSERT(collection.parentCollection() == Collection::root());
            node->parent = collection.parentCollection().id();
            node->type = Node::Collection;
            m_childEntities[collection.parentCollection().id()].prepend(node);

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
    QHashIterator<Collection::Id, QList<Node *> > iter(m_childEntities);
    while (iter.hasNext()) {
        iter.next();
        int nodeIndex = indexOf<Node::Item>(iter.value(), item.id());
        if (nodeIndex != -1 &&
            iter.value().at(nodeIndex)->type == Node::Item) {
            list << m_collections.value(iter.key());
        }
    }

    return list;
}

void EntityTreeModelPrivate::ref(Collection::Id id)
{
    m_monitor->d_ptr->ref(id);
}

bool EntityTreeModelPrivate::shouldPurge(Collection::Id id)
{
    // reference counted collections should never be purged
    // they first have to be deref'ed until they reach 0.
    // if the collection is buffered, keep it.
    if (m_monitor->d_ptr->isMonitored(id)) {
        return false;
    }

    // otherwise we can safely purge this item
    return true;
}

bool EntityTreeModelPrivate::isMonitored(Collection::Id id)
{
    return m_monitor->d_ptr->isMonitored(id);
}

bool EntityTreeModelPrivate::isBuffered(Collection::Id id)
{
    return m_monitor->d_ptr->m_buffer.isBuffered(id);
}

void EntityTreeModelPrivate::deref(Collection::Id id)
{
    const Collection::Id bumpedId = m_monitor->d_ptr->deref(id);

    if (bumpedId < 0) {
        return;
    }

    //The collection has already been removed, don't purge
    if (!m_collections.contains(bumpedId)) {
        return;
    }

    if (shouldPurge(bumpedId)) {
        purgeItems(bumpedId);
    }
}

QList<Node *>::iterator EntityTreeModelPrivate::skipCollections(QList<Node *>::iterator it, QList<Node *>::iterator end, int *pos)
{
    for (; it != end; ++it) {
        if ((*it)->type == Node::Item) {
            break;
        }

        ++(*pos);
    }

    return it;
}

QList<Node *>::iterator EntityTreeModelPrivate::removeItems(QList<Node *>::iterator it, QList<Node *>::iterator end, int *pos, const Collection &collection)
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
    //NOTE: .erase will invalidate all iterators besides "it"!
    for (int i = 0; i < toDelete; ++i) {
        Q_ASSERT(es.count(*it) == 1);
        // don't keep implicitly shared data alive
        Q_ASSERT(m_items.contains((*it)->id));
        m_items.remove((*it)->id);
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
    //if an empty collection is purged and we leave it in here, itemAdded will be ignored for the collection
    //and the collection is never populated by fetchMore (but maybe by statistics changed?)
    m_collectionsWithoutItems.remove(id);
}

void EntityTreeModelPrivate::dataChanged(const QModelIndex &top, const QModelIndex &bottom)
{
    Q_Q(EntityTreeModel);

    QModelIndex rightIndex;

    Node *node = static_cast<Node *>(bottom.internalPointer());

    if (!node) {
        return;
    }

    if (node->type == Node::Collection) {
        rightIndex = bottom.sibling(bottom.row(), q->entityColumnCount(EntityTreeModel::CollectionTreeHeaders) - 1);
    }
    if (node->type == Node::Item) {
        rightIndex = bottom.sibling(bottom.row(), q->entityColumnCount(EntityTreeModel::ItemListHeaders) - 1);
    }

    emit q->dataChanged(top, rightIndex);
}

QModelIndex EntityTreeModelPrivate::indexForCollection(const Collection &collection) const
{
    Q_Q(const EntityTreeModel);

    if (!collection.isValid()) {
        return QModelIndex();
    }

    if (m_collectionFetchStrategy == EntityTreeModel::InvisibleCollectionFetch) {
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
        QHash<Entity::Id, QList<Node *> >::const_iterator it = m_childEntities.constBegin();
        const QHash<Entity::Id, QList<Node *> >::const_iterator end = m_childEntities.constEnd();
        for (; it != end; ++it) {
            const int row = indexOf<Node::Collection>(it.value(), collection.id());
            if (row < 0) {
                continue;
            }

            Node *node = it.value().at(row);
            return q->createIndex(row, 0, static_cast<void *>(node));
        }
        return QModelIndex();
    }

    const int row = indexOf<Node::Collection>(m_childEntities.value(parentId), collection.id());

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
        const int row = indexOf<Node::Item>(nodeList, item.id());
        Q_ASSERT(row >= 0);
        Q_ASSERT(row < nodeList.size());
        Node *node = nodeList.at(row);

        indexes << q->createIndex(row, 0, static_cast<void *>(node));
        return indexes;
    }

    const Collection::List collections = getParentCollections(item);

    foreach (const Collection &collection, collections) {
        const int row = indexOf<Node::Item>(m_childEntities.value(collection.id()), item.id());
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
    foreach (Akonadi::Job *job, m_session->findChildren<Akonadi::Job *>()) {
        job->disconnect(q);
    }
    m_collections.clear();
    m_collectionsWithoutItems.clear();
    m_populatedCols.clear();
    m_items.clear();
    m_pendingCollectionFetchJobs.clear();
    m_pendingCollectionRetrieveJobs.clear();
    m_collectionTreeFetched = false;

    foreach (const QList<Node *> &list, m_childEntities) {
        qDeleteAll(list);
    }
    m_childEntities.clear();
    m_rootNode = 0;

    q->endResetModel();
    fillModel();
}

void EntityTreeModelPrivate::monitoredItemsRetrieved(KJob *job)
{
    if (job->error()) {
        qWarning() << job->errorString();
        return;
    }

    Q_Q(EntityTreeModel);

    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob *>(job);
    Q_ASSERT(fetchJob);
    Item::List list = fetchJob->items();

    q->beginResetModel();
    foreach (const Item &item, list) {
        Node *node = new Node;
        node->id = item.id();
        node->parent = m_rootCollection.id();
        node->type = Node::Item;

        m_childEntities[-1].append(node);
        m_items.insert(item.id(), item);
    }

    q->endResetModel();
}

void EntityTreeModelPrivate::fillModel()
{
    Q_Q(EntityTreeModel);

    m_mimeChecker.setWantedMimeTypes(m_monitor->mimeTypesMonitored());

    const QList<Collection> collections = m_monitor->collectionsMonitored();

    if (collections.isEmpty() &&
        m_monitor->mimeTypesMonitored().isEmpty() &&
        m_monitor->resourcesMonitored().isEmpty() &&
        !m_monitor->itemsMonitoredEx().isEmpty()) {
        m_rootCollection = Collection(-1);
        m_collectionTreeFetched = true;
        emit q_ptr->collectionTreeFetched(collections);     // there are no collections to fetch

        Item::List items;
        foreach (Entity::Id id, m_monitor->itemsMonitoredEx()) {
            items.append(Item(id));
        }
        ItemFetchJob *itemFetch = new ItemFetchJob(items, m_session);
        itemFetch->setFetchScope(m_monitor->itemFetchScope());
        itemFetch->fetchScope().setIgnoreRetrievalErrors(true);
        q->connect(itemFetch, SIGNAL(finished(KJob*)), q, SLOT(monitoredItemsRetrieved(KJob*)));
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
        CollectionFetchJob *rootFetchJob = new CollectionFetchJob(m_rootCollection, CollectionFetchJob::Base, m_session);
        q->connect(rootFetchJob, SIGNAL(result(KJob*)),
                   SLOT(rootFetchJobDone(KJob*)));
        ifDebug(kDebug() << ""; jobTimeTracker[rootFetchJob].start();)
    }
}

bool EntityTreeModelPrivate::canFetchMore(const QModelIndex &parent) const
{
    const Item item = parent.data(EntityTreeModel::ItemRole).value<Item>();

    if (m_collectionFetchStrategy == EntityTreeModel::InvisibleCollectionFetch) {
        return false;
    }

    if (item.isValid()) {
        // items can't have more rows.
        // TODO: Should I use this for fetching more of an item, ie more payload parts?
        return false;
    } else {
        // but collections can...
        const Collection::Id colId = parent.data(EntityTreeModel::CollectionIdRole).toULongLong();

        // But the root collection can't...
        if (Collection::root().id() == colId) {
            return false;
        }

        // Collections which contain no items at all can't contain more
        if (m_collectionsWithoutItems.contains(colId)) {
            return false;
        }

        // Don't start the same job multiple times.
        if (m_pendingCollectionRetrieveJobs.contains(colId)) {
            return false;
        }

        // Can't fetch more if the collection's items have already been fetched
        if (m_populatedCols.contains(colId)) {
            return false;
        }

        foreach (Node *node, m_childEntities.value(colId)) {
            if (Node::Item == node->type) {
                // Only try to fetch more from a collection if we don't already have items in it.
                // Otherwise we'd spend all the time listing items in collections.
                return false;
            }
        }

        return true;
    }
}
