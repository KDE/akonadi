/*
    SPDX-FileCopyrightText: 2008 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "entitytreemodel.h"
#include "akonadicore_debug.h"
#include "entitytreemodel_p.h"
#include "monitor_p.h"

#include <QAbstractProxyModel>
#include <QHash>
#include <QMimeData>

#include <KLocalizedString>
#include <QUrl>
#include <QUrlQuery>

#include "collectionmodifyjob.h"
#include "entitydisplayattribute.h"
#include "itemmodifyjob.h"
#include "monitor.h"
#include "session.h"

#include "collectionutils.h"

#include "pastehelper_p.h"

// clazy:excludeall=old-style-connect

Q_DECLARE_METATYPE(QSet<QByteArray>)

using namespace Akonadi;

EntityTreeModel::EntityTreeModel(Monitor *monitor, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(new EntityTreeModelPrivate(this))
{
    Q_D(EntityTreeModel);
    d->init(monitor);
}

EntityTreeModel::EntityTreeModel(Monitor *monitor, EntityTreeModelPrivate *d, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(d)
{
    d->init(monitor);
}

EntityTreeModel::~EntityTreeModel()
{
    Q_D(EntityTreeModel);

    for (const QList<Node *> &list : std::as_const(d->m_childEntities)) {
        qDeleteAll(list);
    }
}

CollectionFetchScope::ListFilter EntityTreeModel::listFilter() const
{
    Q_D(const EntityTreeModel);
    return d->m_listFilter;
}

void EntityTreeModel::setListFilter(CollectionFetchScope::ListFilter filter)
{
    Q_D(EntityTreeModel);
    d->beginResetModel();
    d->m_listFilter = filter;
    d->m_monitor->setAllMonitored(filter == CollectionFetchScope::NoFilter);
    d->endResetModel();
}

void EntityTreeModel::setCollectionsMonitored(const Collection::List &collections)
{
    Q_D(EntityTreeModel);
    d->beginResetModel();
    const Akonadi::Collection::List lstCols = d->m_monitor->collectionsMonitored();
    for (const Akonadi::Collection &col : lstCols) {
        d->m_monitor->setCollectionMonitored(col, false);
    }
    for (const Akonadi::Collection &col : collections) {
        d->m_monitor->setCollectionMonitored(col, true);
    }
    d->endResetModel();
}

void EntityTreeModel::setCollectionMonitored(const Collection &col, bool monitored)
{
    Q_D(EntityTreeModel);
    d->m_monitor->setCollectionMonitored(col, monitored);
}

bool EntityTreeModel::systemEntitiesShown() const
{
    Q_D(const EntityTreeModel);
    return d->m_showSystemEntities;
}

void EntityTreeModel::setShowSystemEntities(bool show)
{
    Q_D(EntityTreeModel);
    d->m_showSystemEntities = show;
}

void EntityTreeModel::clearAndReset()
{
    Q_D(EntityTreeModel);
    d->beginResetModel();
    d->endResetModel();
}

QHash<int, QByteArray> EntityTreeModel::roleNames() const
{
    return {
        {Qt::DecorationRole, "decoration"},
        {Qt::DisplayRole, "display"},
        {EntityTreeModel::DisplayNameRole, "displayName"},

        {EntityTreeModel::ItemIdRole, "itemId"},
        {EntityTreeModel::ItemRole, "item"},
        {EntityTreeModel::CollectionIdRole, "collectionId"},
        {EntityTreeModel::CollectionRole, "collection"},

        {EntityTreeModel::UnreadCountRole, "unreadCount"},
        {EntityTreeModel::EntityUrlRole, "url"},
        {EntityTreeModel::RemoteIdRole, "remoteId"},
        {EntityTreeModel::IsPopulatedRole, "isPopulated"},
        {EntityTreeModel::CollectionRole, "collection"},
        {EntityTreeModel::MimeTypeRole, "mimeType"},
        {EntityTreeModel::CollectionChildOrderRole, "collectionChildOrder"},
        {EntityTreeModel::ParentCollectionRole, "parentCollection"},
        {EntityTreeModel::SessionRole, "session"},
        {EntityTreeModel::PendingCutRole, "pendingCut"},
        {EntityTreeModel::LoadedPartsRole, "loadedParts"},
        {EntityTreeModel::AvailablePartsRole, "availableParts"},
        {EntityTreeModel::UnreadCountRole, "unreadCount"},
        {EntityTreeModel::FetchStateRole, "fetchState"},
    };
}

int EntityTreeModel::columnCount(const QModelIndex &parent) const
{
    // TODO: Statistics?
    if (parent.isValid() && parent.column() != 0) {
        return 0;
    }

    return qMax(entityColumnCount(CollectionTreeHeaders), entityColumnCount(ItemListHeaders));
}

QVariant EntityTreeModel::entityData(const Item &item, int column, int role) const
{
    Q_D(const EntityTreeModel);

    if (column == 0) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
        case EntityTreeModel::DisplayNameRole:
            if (const auto *attr = item.attribute<EntityDisplayAttribute>(); attr && !attr->displayName().isEmpty()) {
                return attr->displayName();
            } else if (!item.remoteId().isEmpty()) {
                return item.remoteId();
            }
            return QString(QLatin1Char('<') + QString::number(item.id()) + u'>');
        case Qt::DecorationRole:
            if (const auto *attr = item.attribute<EntityDisplayAttribute>(); attr && !attr->iconName().isEmpty()) {
                return d->iconForName(attr->iconName());
            }
            break;
        default:
            break;
        }
    }

    return QVariant();
}

QVariant EntityTreeModel::entityData(const Collection &collection, int column, int role) const
{
    Q_D(const EntityTreeModel);

    if (column != 0) {
        return QString();
    }

    if (collection == Collection::root()) {
        // Only display the root collection. It may not be edited.
        if (role == Qt::DisplayRole || role == EntityTreeModel::DisplayNameRole) {
            return d->m_rootCollectionDisplayName;
        } else if (role == Qt::EditRole) {
            return QVariant();
        }
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case EntityTreeModel::DisplayNameRole:
        if (column == 0) {
            if (const QString displayName = collection.displayName(); !displayName.isEmpty()) {
                return displayName;
            } else {
                return i18nc("@info:status", "Loading...");
            }
        }
        break;
    case Qt::DecorationRole:
        if (const auto *const attr = collection.attribute<EntityDisplayAttribute>(); attr && !attr->iconName().isEmpty()) {
            return d->iconForName(attr->iconName());
        }
        return d->iconForName(CollectionUtils::defaultIconName(collection));
    default:
        break;
    }

    return QVariant();
}

QVariant EntityTreeModel::data(const QModelIndex &index, int role) const
{
    Q_D(const EntityTreeModel);
    if (role == SessionRole) {
        return QVariant::fromValue(qobject_cast<QObject *>(d->m_session));
    }

    // Ugly, but at least the API is clean.
    const auto headerGroup = static_cast<HeaderGroup>((role / static_cast<int>(TerminalUserRole)));

    role %= TerminalUserRole;
    if (!index.isValid()) {
        if (ColumnCountRole != role) {
            return QVariant();
        }

        return entityColumnCount(headerGroup);
    }

    if (ColumnCountRole == role) {
        return entityColumnCount(headerGroup);
    }

    const Node *node = reinterpret_cast<Node *>(index.internalPointer());

    if (ParentCollectionRole == role && d->m_collectionFetchStrategy != FetchNoCollections) {
        const Collection parentCollection = d->m_collections.value(node->parent);
        Q_ASSERT(parentCollection.isValid());

        return QVariant::fromValue(parentCollection);
    }

    if (Node::Collection == node->type) {
        const Collection collection = d->m_collections.value(node->id);
        if (!collection.isValid()) {
            return QVariant();
        }

        switch (role) {
        case MimeTypeRole:
            return collection.mimeType();
        case RemoteIdRole:
            return collection.remoteId();
        case CollectionIdRole:
            return collection.id();
        case ItemIdRole:
            // QVariant().toInt() is 0, not -1, so we have to handle the ItemIdRole
            // and CollectionIdRole (below) specially
            return -1;
        case CollectionRole:
            return QVariant::fromValue(collection);
        case EntityUrlRole:
            return collection.url().url();
        case UnreadCountRole:
            return collection.statistics().unreadCount();
        case FetchStateRole:
            return d->m_pendingCollectionRetrieveJobs.contains(collection.id()) ? FetchingState : IdleState;
        case IsPopulatedRole:
            return d->m_populatedCols.contains(collection.id());
        case OriginalCollectionNameRole:
            return entityData(collection, index.column(), Qt::DisplayRole);
        case PendingCutRole:
            return d->m_pendingCutCollections.contains(node->id);
        case Qt::BackgroundRole:
            if (const auto *const attr = collection.attribute<EntityDisplayAttribute>(); attr && attr->backgroundColor().isValid()) {
                return attr->backgroundColor();
            }
            [[fallthrough]];
        default:
            return entityData(collection, index.column(), role);
        }

    } else if (Node::Item == node->type) {
        const Item item = d->m_items.value(node->id);
        if (!item.isValid()) {
            return QVariant();
        }

        switch (role) {
        case ParentCollectionRole:
            return QVariant::fromValue(item.parentCollection());
        case MimeTypeRole:
            return item.mimeType();
        case RemoteIdRole:
            return item.remoteId();
        case ItemRole:
            return QVariant::fromValue(item);
        case ItemIdRole:
            return item.id();
        case CollectionIdRole:
            return -1;
        case LoadedPartsRole:
            return QVariant::fromValue(item.loadedPayloadParts());
        case AvailablePartsRole:
            return QVariant::fromValue(item.availablePayloadParts());
        case EntityUrlRole:
            return item.url(Akonadi::Item::UrlWithMimeType).url();
        case PendingCutRole:
            return d->m_pendingCutItems.contains(node->id);
        case Qt::BackgroundRole:
            if (const auto *const attr = item.attribute<EntityDisplayAttribute>(); attr && attr->backgroundColor().isValid()) {
                return attr->backgroundColor();
            }
            [[fallthrough]];
        default:
            return entityData(item, index.column(), role);
        }
    }

    return QVariant();
}

Qt::ItemFlags EntityTreeModel::flags(const QModelIndex &index) const
{
    Q_D(const EntityTreeModel);
    // Pass modeltest.
    if (!index.isValid()) {
        return {};
    }

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    const Node *node = reinterpret_cast<Node *>(index.internalPointer());

    if (Node::Collection == node->type) {
        const Collection collection = d->m_collections.value(node->id);
        if (collection.isValid()) {
            if (collection == Collection::root()) {
                // Selectable and displayable only.
                return flags;
            }

            const int rights = collection.rights();

            if (rights & Collection::CanChangeCollection) {
                if (index.column() == 0) {
                    flags |= Qt::ItemIsEditable;
                }
                // Changing the collection includes changing the metadata (child entityordering).
                // Need to allow this by drag and drop.
                flags |= Qt::ItemIsDropEnabled;
            }
            if (rights & (Collection::CanCreateCollection | Collection::CanCreateItem | Collection::CanLinkItem)) {
                // Can we drop new collections and items into this collection?
                flags |= Qt::ItemIsDropEnabled;
            }

            // dragging is always possible, even for read-only objects, but they can only be copied, not moved.
            flags |= Qt::ItemIsDragEnabled;
        }
    } else if (Node::Item == node->type) {
        // cut out entities are shown as disabled
        // TODO: Not sure this is wanted, it prevents any interaction with them, better
        // solution would be to move this to the delegate, as was done for collections.
        if (d->m_pendingCutItems.contains(node->id)) {
            return Qt::ItemIsSelectable;
        }

        // Rights come from the parent collection.

        Collection parentCollection;
        if (!index.parent().isValid()) {
            parentCollection = d->m_rootCollection;
        } else {
            const Node *parentNode = reinterpret_cast<Node *>(index.parent().internalPointer());
            parentCollection = d->m_collections.value(parentNode->id);
        }
        if (parentCollection.isValid()) {
            const int rights = parentCollection.rights();

            // Can't drop onto items.
            if (rights & Collection::CanChangeItem && index.column() == 0) {
                flags |= Qt::ItemIsEditable;
            }
            // dragging is always possible, even for read-only objects, but they can only be copied, not moved.
            flags |= Qt::ItemIsDragEnabled;
        }
    }

    return flags;
}

Qt::DropActions EntityTreeModel::supportedDropActions() const
{
    return (Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
}

QStringList EntityTreeModel::mimeTypes() const
{
    // TODO: Should this return the mimetypes that the items provide? Allow dragging a contact from here for example.
    return {QStringLiteral("text/uri-list")};
}

bool EntityTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row)
    Q_UNUSED(column)
    Q_D(EntityTreeModel);

    // Can't drop onto Collection::root.
    if (!parent.isValid()) {
        return false;
    }

    // TODO Use action and collection rights and return false if necessary

    // if row and column are -1, then the drop was on parent directly.
    // data should then be appended on the end of the items of the collections as appropriate.
    // That will mean begin insert rows etc.
    // Otherwise it was a sibling of the row^th item of parent.
    // Needs to be handled when ordering is accounted for.

    // Handle dropping between items as well as on items.
    //   if ( row != -1 && column != -1 )
    //   {
    //   }

    if (action == Qt::IgnoreAction) {
        return true;
    }

    // Shouldn't do this. Need to be able to drop vcards for example.
    //   if ( !data->hasFormat( "text/uri-list" ) )
    //       return false;

    Node *node = reinterpret_cast<Node *>(parent.internalId());

    Q_ASSERT(node);

    if (Node::Item == node->type) {
        if (!parent.parent().isValid()) {
            // The drop is somehow on an item with no parent (shouldn't happen)
            // The drop should be considered handled anyway.
            qCWarning(AKONADICORE_LOG) << "Dropped onto item with no parent collection";
            return true;
        }

        // A drop onto an item should be considered as a drop onto its parent collection
        node = reinterpret_cast<Node *>(parent.parent().internalId());
    }

    if (Node::Collection == node->type) {
        const Collection destCollection = d->m_collections.value(node->id);

        // Applications can't create new collections in root. Only resources can.
        if (destCollection == Collection::root()) {
            // Accept the event so that it doesn't propagate.
            return true;
        }

        if (data->hasFormat(QStringLiteral("text/uri-list"))) {
            MimeTypeChecker mimeChecker;
            mimeChecker.setWantedMimeTypes(destCollection.contentMimeTypes());

            const QList<QUrl> urls = data->urls();
            for (const QUrl &url : urls) {
                const Collection collection = d->m_collections.value(Collection::fromUrl(url).id());
                if (collection.isValid()) {
                    if (collection.parentCollection().id() == destCollection.id() && action != Qt::CopyAction) {
                        qCWarning(AKONADICORE_LOG) << "Error: source and destination of move are the same.";
                        return false;
                    }

                    if (!mimeChecker.isWantedCollection(collection)) {
                        qCDebug(AKONADICORE_LOG) << "unwanted collection" << mimeChecker.wantedMimeTypes() << collection.contentMimeTypes();
                        return false;
                    }

                    QUrlQuery query(url);
                    if (query.hasQueryItem(QStringLiteral("name"))) {
                        const QString collectionName = query.queryItemValue(QStringLiteral("name"));
                        const QStringList collectionNames = d->childCollectionNames(destCollection);

                        if (collectionNames.contains(collectionName)) {
                            Q_EMIT errorOccurred(
                                i18n("The target collection '%1' contains already\na collection with name '%2'.", destCollection.name(), collection.name()));
                            return false;
                        }
                    }
                } else {
                    const Item item = d->m_items.value(Item::fromUrl(url).id());
                    if (item.isValid()) {
                        if (item.parentCollection().id() == destCollection.id() && action != Qt::CopyAction) {
                            qCWarning(AKONADICORE_LOG) << "Error: source and destination of move are the same.";
                            return false;
                        }

                        if (!mimeChecker.isWantedItem(item)) {
                            qCDebug(AKONADICORE_LOG) << "unwanted item" << mimeChecker.wantedMimeTypes() << item.mimeType();
                            return false;
                        }
                    }
                }
            }

            KJob *job = PasteHelper::pasteUriList(data, destCollection, action, d->m_session);
            if (!job) {
                return false;
            }

            connect(job, SIGNAL(result(KJob *)), SLOT(pasteJobDone(KJob *)));

            // Accept the event so that it doesn't propagate.
            return true;
        } else {
            //       not a set of uris. Maybe vcards etc. Check if the parent supports them, and maybe do
            // fromMimeData for them. Hmm, put it in the same transaction with the above?
            // TODO: This should be handled first, not last.
        }
    }

    return false;
}

QModelIndex EntityTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_D(const EntityTreeModel);

    if (parent.column() > 0) {
        return QModelIndex();
    }

    // TODO: don't use column count here? Use some d-> func.
    if (column >= columnCount() || column < 0) {
        return QModelIndex();
    }

    QList<Node *> childEntities;

    const Node *parentNode = reinterpret_cast<Node *>(parent.internalPointer());
    if (!parentNode || !parent.isValid()) {
        if (d->m_showRootCollection) {
            childEntities << d->m_childEntities.value(-1);
        } else {
            childEntities = d->m_childEntities.value(d->m_rootCollection.id());
        }
    } else if (parentNode->id >= 0) {
        childEntities = d->m_childEntities.value(parentNode->id);
    }

    const int size = childEntities.size();
    if (row < 0 || row >= size) {
        return QModelIndex();
    }

    Node *node = childEntities.at(row);
    return createIndex(row, column, reinterpret_cast<void *>(node));
}

QModelIndex EntityTreeModel::parent(const QModelIndex &index) const
{
    Q_D(const EntityTreeModel);

    if (!index.isValid()) {
        return QModelIndex();
    }

    if (d->m_collectionFetchStrategy == InvisibleCollectionFetch || d->m_collectionFetchStrategy == FetchNoCollections) {
        return QModelIndex();
    }

    const Node *node = reinterpret_cast<Node *>(index.internalPointer());

    if (!node) {
        return QModelIndex();
    }

    const Collection collection = d->m_collections.value(node->parent);

    if (!collection.isValid()) {
        return QModelIndex();
    }

    if (collection.id() == d->m_rootCollection.id()) {
        if (!d->m_showRootCollection) {
            return QModelIndex();
        } else {
            return createIndex(0, 0, reinterpret_cast<void *>(d->m_rootNode));
        }
    }

    Q_ASSERT(collection.parentCollection().isValid());
    const int row = d->indexOf<Node::Collection>(d->m_childEntities.value(collection.parentCollection().id()), collection.id());

    Q_ASSERT(row >= 0);
    Node *parentNode = d->m_childEntities.value(collection.parentCollection().id()).at(row);

    return createIndex(row, 0, reinterpret_cast<void *>(parentNode));
}

int EntityTreeModel::rowCount(const QModelIndex &parent) const
{
    Q_D(const EntityTreeModel);

    if (d->m_collectionFetchStrategy == InvisibleCollectionFetch || d->m_collectionFetchStrategy == FetchNoCollections) {
        if (parent.isValid()) {
            return 0;
        } else {
            return d->m_items.size();
        }
    }

    if (!parent.isValid()) {
        // If we're showing the root collection then it will be the only child of the root.
        if (d->m_showRootCollection) {
            return d->m_childEntities.value(-1).size();
        }
        return d->m_childEntities.value(d->m_rootCollection.id()).size();
    }

    if (parent.column() != 0) {
        return 0;
    }

    const Node *node = reinterpret_cast<Node *>(parent.internalPointer());

    if (!node) {
        return 0;
    }

    if (Node::Item == node->type) {
        return 0;
    }

    Q_ASSERT(parent.isValid());
    return d->m_childEntities.value(node->id).size();
}

int EntityTreeModel::entityColumnCount(HeaderGroup headerGroup) const
{
    // Not needed in this model.
    Q_UNUSED(headerGroup)

    return 1;
}

QVariant EntityTreeModel::entityHeaderData(int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup) const
{
    Q_D(const EntityTreeModel);
    // Not needed in this model.
    Q_UNUSED(headerGroup)

    if (section == 0 && orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == EntityTreeModel::DisplayNameRole)) {
        if (d->m_rootCollection == Collection::root()) {
            return i18nc("@title:column Name of a thing", "Name");
        }
        return d->m_rootCollection.name();
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant EntityTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    const auto headerGroup = static_cast<HeaderGroup>((role / static_cast<int>(TerminalUserRole)));

    role %= TerminalUserRole;
    return entityHeaderData(section, orientation, role, headerGroup);
}

QMimeData *EntityTreeModel::mimeData(const QModelIndexList &indexes) const
{
    Q_D(const EntityTreeModel);

    auto data = new QMimeData();
    QList<QUrl> urls;
    for (const QModelIndex &index : indexes) {
        if (index.column() != 0) {
            continue;
        }

        if (!index.isValid()) {
            continue;
        }

        const Node *node = reinterpret_cast<Node *>(index.internalPointer());

        if (Node::Collection == node->type) {
            urls << d->m_collections.value(node->id).url(Collection::UrlWithName);
        } else if (Node::Item == node->type) {
            QUrl url = d->m_items.value(node->id).url(Item::Item::UrlWithMimeType);
            QUrlQuery query(url);
            query.addQueryItem(QStringLiteral("parent"), QString::number(node->parent));
            url.setQuery(query);
            urls << url;
        } else { // if that happens something went horrible wrong
            Q_ASSERT(false);
        }
    }

    data->setUrls(urls);

    return data;
}

// Always return false for actions which take place asynchronously, eg via a Job.
bool EntityTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(EntityTreeModel);

    const Node *node = reinterpret_cast<Node *>(index.internalPointer());

    if (role == PendingCutRole) {
        if (index.isValid() && value.toBool()) {
            if (Node::Collection == node->type) {
                d->m_pendingCutCollections.append(node->id);
            } else if (Node::Item == node->type) {
                d->m_pendingCutItems.append(node->id);
            }
        } else {
            d->m_pendingCutCollections.clear();
            d->m_pendingCutItems.clear();
        }
        return true;
    }

    if (index.isValid() && node->type == Node::Collection && (role == CollectionRefRole || role == CollectionDerefRole)) {
        const Collection collection = index.data(CollectionRole).value<Collection>();
        Q_ASSERT(collection.isValid());

        if (role == CollectionDerefRole) {
            d->deref(collection.id());
        } else if (role == CollectionRefRole) {
            d->ref(collection.id());
        }
        return true;
    }

    if (index.column() == 0 && (role & (Qt::EditRole | ItemRole | CollectionRole))) {
        if (Node::Collection == node->type) {
            Collection collection = d->m_collections.value(node->id);
            if (!collection.isValid() || !value.isValid()) {
                return false;
            }

            if (Qt::EditRole == role) {
                collection.setName(value.toString());
                if (collection.hasAttribute<EntityDisplayAttribute>()) {
                    auto *displayAttribute = collection.attribute<EntityDisplayAttribute>();
                    displayAttribute->setDisplayName(value.toString());
                }
            } else if (Qt::BackgroundRole == role) {
                auto color = value.value<QColor>();
                if (!color.isValid()) {
                    return false;
                }

                auto *eda = collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing);
                eda->setBackgroundColor(color);
            } else if (CollectionRole == role) {
                collection = value.value<Collection>();
            }

            auto job = new CollectionModifyJob(collection, d->m_session);
            connect(job, &CollectionModifyJob::result, this, [d](KJob *j) {
                d->updateJobDone(j);
            });

            return false;
        } else if (Node::Item == node->type) {
            Item item = d->m_items.value(node->id);
            if (!item.isValid() || !value.isValid()) {
                return false;
            }

            if (Qt::EditRole == role) {
                if (item.hasAttribute<EntityDisplayAttribute>()) {
                    auto *displayAttribute = item.attribute<EntityDisplayAttribute>(Item::AddIfMissing);
                    displayAttribute->setDisplayName(value.toString());
                }
            } else if (Qt::BackgroundRole == role) {
                auto color = value.value<QColor>();
                if (!color.isValid()) {
                    return false;
                }

                auto *eda = item.attribute<EntityDisplayAttribute>(Item::AddIfMissing);
                eda->setBackgroundColor(color);
            } else if (ItemRole == role) {
                item = value.value<Item>();
                Q_ASSERT(item.id() == node->id);
            }

            auto itemModifyJob = new ItemModifyJob(item, d->m_session);
            connect(itemModifyJob, &CollectionModifyJob::result, this, [d](KJob *j) {
                d->updateJobDone(j);
            });

            return false;
        }
    }

    return QAbstractItemModel::setData(index, value, role);
}

bool EntityTreeModel::canFetchMore(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return false;
}

void EntityTreeModel::fetchMore(const QModelIndex &parent)
{
    Q_D(EntityTreeModel);

    if (!d->canFetchMore(parent)) {
        return;
    }

    if (d->m_collectionFetchStrategy == InvisibleCollectionFetch) {
        return;
    }

    if (d->m_itemPopulation == ImmediatePopulation) {
        // Nothing to do. The items are already in the model.
        return;
    } else if (d->m_itemPopulation == LazyPopulation) {
        const Collection collection = parent.data(CollectionRole).value<Collection>();

        if (!collection.isValid()) {
            return;
        }

        d->fetchItems(collection);
    }
}

bool EntityTreeModel::hasChildren(const QModelIndex &parent) const
{
    Q_D(const EntityTreeModel);

    if (d->m_collectionFetchStrategy == InvisibleCollectionFetch || d->m_collectionFetchStrategy == FetchNoCollections) {
        return parent.isValid() ? false : !d->m_items.isEmpty();
    }

    // TODO: Empty collections right now will return true and get a little + to expand.
    // There is probably no way to tell if a collection
    // has child items in akonadi without first attempting an itemFetchJob...
    // Figure out a way to fix this. (Statistics)
    return ((rowCount(parent) > 0) || (d->canFetchMore(parent) && d->m_itemPopulation == LazyPopulation));
}

bool EntityTreeModel::isCollectionTreeFetched() const
{
    Q_D(const EntityTreeModel);
    return d->m_collectionTreeFetched;
}

bool EntityTreeModel::isCollectionPopulated(Collection::Id id) const
{
    Q_D(const EntityTreeModel);
    return d->m_populatedCols.contains(id);
}

bool EntityTreeModel::isFullyPopulated() const
{
    Q_D(const EntityTreeModel);
    return d->m_collectionTreeFetched && d->m_pendingCollectionRetrieveJobs.isEmpty();
}

QModelIndexList EntityTreeModel::match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
{
    Q_D(const EntityTreeModel);

    if (role == CollectionIdRole || role == CollectionRole) {
        Collection::Id id;
        if (role == CollectionRole) {
            const Collection collection = value.value<Collection>();
            id = collection.id();
        } else {
            id = value.toLongLong();
        }

        const Collection collection = d->m_collections.value(id);
        if (!collection.isValid()) {
            return {};
        }

        const QModelIndex collectionIndex = d->indexForCollection(collection);
        Q_ASSERT(collectionIndex.isValid());
        return {collectionIndex};
    } else if (role == ItemIdRole || role == ItemRole) {
        Item::Id id;
        if (role == ItemRole) {
            id = value.value<Item>().id();
        } else {
            id = value.toLongLong();
        }

        const Item item = d->m_items.value(id);
        if (!item.isValid()) {
            return {};
        }
        return d->indexesForItem(item);
    } else if (role == EntityUrlRole) {
        const QUrl url(value.toString());
        const Item item = Item::fromUrl(url);

        if (item.isValid()) {
            return d->indexesForItem(d->m_items.value(item.id()));
        }

        const Collection collection = Collection::fromUrl(url);
        if (!collection.isValid()) {
            return {};
        }
        return {d->indexForCollection(collection)};
    }

    return QAbstractItemModel::match(start, role, value, hits, flags);
}

bool EntityTreeModel::insertRows(int /*row*/, int /*count*/, const QModelIndex & /*parent*/)
{
    return false;
}

bool EntityTreeModel::insertColumns(int /*column*/, int /*count*/, const QModelIndex & /*parent*/)
{
    return false;
}

bool EntityTreeModel::removeRows(int /*row*/, int /*count*/, const QModelIndex & /*parent*/)
{
    return false;
}

bool EntityTreeModel::removeColumns(int /*column*/, int /*count*/, const QModelIndex & /*parent*/)
{
    return false;
}

void EntityTreeModel::setItemPopulationStrategy(ItemPopulationStrategy strategy)
{
    Q_D(EntityTreeModel);
    d->beginResetModel();
    d->m_itemPopulation = strategy;

    if (strategy == NoItemPopulation) {
        disconnect(d->m_monitor, SIGNAL(itemAdded(Akonadi::Item, Akonadi::Collection)), this, SLOT(monitoredItemAdded(Akonadi::Item, Akonadi::Collection)));
        disconnect(d->m_monitor, SIGNAL(itemChanged(Akonadi::Item, QSet<QByteArray>)), this, SLOT(monitoredItemChanged(Akonadi::Item, QSet<QByteArray>)));
        disconnect(d->m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), this, SLOT(monitoredItemRemoved(Akonadi::Item)));
        disconnect(d->m_monitor,
                   SIGNAL(itemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)),
                   this,
                   SLOT(monitoredItemMoved(Akonadi::Item, Akonadi::Collection, Akonadi::Collection)));

        disconnect(d->m_monitor, SIGNAL(itemLinked(Akonadi::Item, Akonadi::Collection)), this, SLOT(monitoredItemLinked(Akonadi::Item, Akonadi::Collection)));
        disconnect(d->m_monitor,
                   SIGNAL(itemUnlinked(Akonadi::Item, Akonadi::Collection)),
                   this,
                   SLOT(monitoredItemUnlinked(Akonadi::Item, Akonadi::Collection)));
    }

    d->m_monitor->d_ptr->useRefCounting = (strategy == LazyPopulation);

    d->endResetModel();
}

EntityTreeModel::ItemPopulationStrategy EntityTreeModel::itemPopulationStrategy() const
{
    Q_D(const EntityTreeModel);
    return d->m_itemPopulation;
}

void EntityTreeModel::setIncludeRootCollection(bool include)
{
    Q_D(EntityTreeModel);
    d->beginResetModel();
    d->m_showRootCollection = include;
    d->endResetModel();
}

bool EntityTreeModel::includeRootCollection() const
{
    Q_D(const EntityTreeModel);
    return d->m_showRootCollection;
}

void EntityTreeModel::setRootCollectionDisplayName(const QString &displayName)
{
    Q_D(EntityTreeModel);
    d->m_rootCollectionDisplayName = displayName;

    // TODO: Emit datachanged if it is being shown.
}

QString EntityTreeModel::rootCollectionDisplayName() const
{
    Q_D(const EntityTreeModel);
    return d->m_rootCollectionDisplayName;
}

void EntityTreeModel::setCollectionFetchStrategy(CollectionFetchStrategy strategy)
{
    Q_D(EntityTreeModel);
    d->beginResetModel();
    d->m_collectionFetchStrategy = strategy;

    if (strategy == FetchNoCollections || strategy == InvisibleCollectionFetch) {
        disconnect(d->m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)), this, SLOT(monitoredCollectionChanged(Akonadi::Collection)));
        disconnect(d->m_monitor,
                   SIGNAL(collectionAdded(Akonadi::Collection, Akonadi::Collection)),
                   this,
                   SLOT(monitoredCollectionAdded(Akonadi::Collection, Akonadi::Collection)));
        disconnect(d->m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)), this, SLOT(monitoredCollectionRemoved(Akonadi::Collection)));
        disconnect(d->m_monitor,
                   SIGNAL(collectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)),
                   this,
                   SLOT(monitoredCollectionMoved(Akonadi::Collection, Akonadi::Collection, Akonadi::Collection)));
        d->m_monitor->fetchCollection(false);
    } else {
        d->m_monitor->fetchCollection(true);
    }

    d->endResetModel();
}

EntityTreeModel::CollectionFetchStrategy EntityTreeModel::collectionFetchStrategy() const
{
    Q_D(const EntityTreeModel);
    return d->m_collectionFetchStrategy;
}

static QPair<QList<const QAbstractProxyModel *>, const EntityTreeModel *> proxiesAndModel(const QAbstractItemModel *model)
{
    QList<const QAbstractProxyModel *> proxyChain;
    const auto *proxy = qobject_cast<const QAbstractProxyModel *>(model);
    const QAbstractItemModel *_model = model;
    while (proxy) {
        proxyChain.prepend(proxy);
        _model = proxy->sourceModel();
        proxy = qobject_cast<const QAbstractProxyModel *>(_model);
    }

    const auto *etm = qobject_cast<const EntityTreeModel *>(_model);
    return qMakePair(proxyChain, etm);
}

static QModelIndex proxiedIndex(const QModelIndex &idx, const QList<const QAbstractProxyModel *> &proxyChain)
{
    QModelIndex _idx = idx;
    for (const auto *proxy : proxyChain) {
        _idx = proxy->mapFromSource(_idx);
    }
    return _idx;
}

QModelIndex EntityTreeModel::modelIndexForCollection(const QAbstractItemModel *model, const Collection &collection)
{
    const auto &[proxy, etm] = proxiesAndModel(model);
    if (!etm) {
        qCWarning(AKONADICORE_LOG) << "Model" << model << "is not derived from ETM or a proxy model on top of ETM.";
        return {};
    }

    QModelIndex idx = etm->d_ptr->indexForCollection(collection);
    return proxiedIndex(idx, proxy);
}

QModelIndexList EntityTreeModel::modelIndexesForItem(const QAbstractItemModel *model, const Item &item)
{
    const auto &[proxy, etm] = proxiesAndModel(model);

    if (!etm) {
        qCWarning(AKONADICORE_LOG) << "Model" << model << "is not derived from ETM or a proxy model on top of ETM.";
        return QModelIndexList();
    }

    const QModelIndexList list = etm->d_ptr->indexesForItem(item);
    QModelIndexList proxyList;
    for (const QModelIndex &idx : list) {
        const QModelIndex pIdx = proxiedIndex(idx, proxy);
        if (pIdx.isValid()) {
            proxyList.push_back(pIdx);
        }
    }
    return proxyList;
}

Collection EntityTreeModel::updatedCollection(const QAbstractItemModel *model, qint64 collectionId)
{
    const auto *proxy = qobject_cast<const QAbstractProxyModel *>(model);
    const QAbstractItemModel *_model = model;
    while (proxy) {
        _model = proxy->sourceModel();
        proxy = qobject_cast<const QAbstractProxyModel *>(_model);
    }

    const auto *etm = qobject_cast<const EntityTreeModel *>(_model);
    if (etm) {
        return etm->d_ptr->m_collections.value(collectionId);
    } else {
        return Collection{collectionId};
    }
}

Collection EntityTreeModel::updatedCollection(const QAbstractItemModel *model, const Collection &collection)
{
    return updatedCollection(model, collection.id());
}

#include "moc_entitytreemodel.cpp"
