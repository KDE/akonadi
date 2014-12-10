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

#include "entitytreemodel.h"
#include "entitytreemodel_p.h"

#include "monitor_p.h"

#include <QtCore/QHash>
#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QAbstractProxyModel>

#include <KDE/KIcon>
#include <KDE/KLocalizedString>
#include <KDE/KMessageBox>
#include <KDE/KUrl>

#include <akonadi/attributefactory.h>
#include <akonadi/changerecorder.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/entitydisplayattribute.h>
#include <akonadi/transactionsequence.h>
#include <akonadi/itemmodifyjob.h>
#include <akonadi/session.h>
#include "collectionfetchscope.h"

#include "collectionutils_p.h"

#include "kdebug.h"
#include "pastehelper_p.h"

Q_DECLARE_METATYPE(QSet<QByteArray>)

using namespace Akonadi;

EntityTreeModel::EntityTreeModel(ChangeRecorder *monitor, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(new EntityTreeModelPrivate(this))
{
    Q_D(EntityTreeModel);
    d->init(monitor);
}

EntityTreeModel::EntityTreeModel(ChangeRecorder *monitor, EntityTreeModelPrivate *d, QObject *parent)
    : QAbstractItemModel(parent)
    , d_ptr(d)
{
    d->init(monitor);
}

EntityTreeModel::~EntityTreeModel()
{
    Q_D(EntityTreeModel);

    foreach (const QList<Node *> &list, d->m_childEntities) {
        QList<Node *>::const_iterator it = list.constBegin();
        const QList<Node *>::const_iterator end = list.constEnd();
        for (; it != end; ++it) {
            delete *it;
        }
    }

    d->m_rootNode = 0;

    delete d_ptr;
}

bool EntityTreeModel::includeUnsubscribed() const
{
    return (listFilter() == CollectionFetchScope::NoFilter);
}

void EntityTreeModel::setIncludeUnsubscribed(bool show)
{
    if (show) {
        setListFilter(CollectionFetchScope::NoFilter);
    } else {
        setListFilter(CollectionFetchScope::Enabled);
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
    foreach(const Akonadi::Collection &col, d->m_monitor->collectionsMonitored()) {
        d->m_monitor->setCollectionMonitored(col, false);
    }
    foreach(const Akonadi::Collection &col, collections) {
        d->m_monitor->setCollectionMonitored(col, true);
    }
    d->endResetModel();
}

void EntityTreeModel::setCollectionMonitored(const Collection &col, bool monitored)
{
    Q_D(EntityTreeModel);
    d->m_monitor->setCollectionMonitored(col, monitored);
}

void EntityTreeModel::setCollectionReferenced(const Akonadi::Collection &col, bool referenced)
{
    Q_D(EntityTreeModel);
    Akonadi::Collection referencedCollection = col;
    referencedCollection.setReferenced(referenced);
    //We have to use the same session as the monitor, so the monitor can fetch the collection afterwards
    new Akonadi::CollectionModifyJob(referencedCollection, d->m_monitor->session());
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

int EntityTreeModel::columnCount(const QModelIndex &parent) const
{
// TODO: Statistics?
    if (parent.isValid() &&
        parent.column() != 0) {
        return 0;
    }

    return qMax(entityColumnCount(CollectionTreeHeaders), entityColumnCount(ItemListHeaders));
}

QVariant EntityTreeModel::entityData(const Item &item, int column, int role) const
{
    if (column == 0) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::EditRole:
            if (item.hasAttribute<EntityDisplayAttribute>() &&
                !item.attribute<EntityDisplayAttribute>()->displayName().isEmpty()) {
                return item.attribute<EntityDisplayAttribute>()->displayName();
            } else {
                if (!item.remoteId().isEmpty()) {
                    return item.remoteId();
                }
                return QString(QLatin1String("<") + QString::number(item.id()) + QLatin1String(">"));
            }
            break;
        case Qt::DecorationRole:
            if (item.hasAttribute<EntityDisplayAttribute>() &&
                !item.attribute<EntityDisplayAttribute>()->iconName().isEmpty()) {
                return item.attribute<EntityDisplayAttribute>()->icon();
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

    if (column > 0) {
        return QString();
    }

    if (collection == Collection::root()) {
        // Only display the root collection. It may not be edited.
        if (role == Qt::DisplayRole) {
            return d->m_rootCollectionDisplayName;
        }

        if (role == Qt::EditRole) {
            return QVariant();
        }
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::EditRole:
        if (column == 0) {
            const QString displayName = collection.displayName();
            if (!displayName.isEmpty()) {
                return displayName;
            } else {
                return i18n("Loading...");
            }
        }
        break;
    case Qt::DecorationRole:
        if (collection.hasAttribute<EntityDisplayAttribute>() &&
            !collection.attribute<EntityDisplayAttribute>()->iconName().isEmpty()) {
            return collection.attribute<EntityDisplayAttribute>()->icon();
        }
        return KIcon(CollectionUtils::defaultIconName(collection));
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
    const HeaderGroup headerGroup = static_cast<HeaderGroup>((role / static_cast<int>(TerminalUserRole)));

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

    if (ParentCollectionRole == role &&
        d->m_collectionFetchStrategy != FetchNoCollections) {
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
            break;
        case RemoteIdRole:
            return collection.remoteId();
            break;
        case CollectionIdRole:
            return collection.id();
            break;
        case ItemIdRole:
            // QVariant().toInt() is 0, not -1, so we have to handle the ItemIdRole
            // and CollectionIdRole (below) specially
            return -1;
            break;
        case CollectionRole:
            return QVariant::fromValue(collection);
            break;
        case EntityUrlRole:
            return collection.url().url();
            break;
        case UnreadCountRole: {
            CollectionStatistics statistics = collection.statistics();
            return statistics.unreadCount();
        }
        case FetchStateRole: {
            return d->m_pendingCollectionRetrieveJobs.contains(collection.id()) ? FetchingState : IdleState;
        }
        case CollectionSyncProgressRole: {
            return QVariant(); // no longer supported
        }
        case IsPopulatedRole: {
            return d->m_populatedCols.contains(collection.id());
        }
        case Qt::BackgroundRole: {
            if (collection.hasAttribute<EntityDisplayAttribute>()) {
                EntityDisplayAttribute *eda = collection.attribute<EntityDisplayAttribute>();
                QColor color = eda->backgroundColor();
                if (color.isValid()) {
                    return color;
                }
            }
            // fall through.
        }
        default:
            return entityData(collection, index.column(), role);
            break;
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
            break;
        case RemoteIdRole:
            return item.remoteId();
            break;
        case ItemRole:
            return QVariant::fromValue(item);
            break;
        case ItemIdRole:
            return item.id();
            break;
        case CollectionIdRole:
            return -1;
            break;
        case LoadedPartsRole:
            return QVariant::fromValue(item.loadedPayloadParts());
            break;
        case AvailablePartsRole:
            return QVariant::fromValue(item.availablePayloadParts());
            break;
        case EntityUrlRole:
            return item.url(Akonadi::Item::UrlWithMimeType).url();
            break;
        case Qt::BackgroundRole: {
            if (item.hasAttribute<EntityDisplayAttribute>()) {
                EntityDisplayAttribute *eda = item.attribute<EntityDisplayAttribute>();
                const QColor color = eda->backgroundColor();
                if (color.isValid()) {
                    return color;
                }
            }
            // fall through.
        }
        default:
            return entityData(item, index.column(), role);
            break;
        }
    }

    return QVariant();
}

Qt::ItemFlags EntityTreeModel::flags(const QModelIndex &index) const
{
    Q_D(const EntityTreeModel);
    // Pass modeltest.
    if (!index.isValid()) {
        return 0;
    }

    Qt::ItemFlags flags = QAbstractItemModel::flags(index);

    const Node *node = reinterpret_cast<Node *>(index.internalPointer());

    if (Node::Collection == node->type) {
        // cut out entities will be shown as inactive
        if (d->m_pendingCutCollections.contains(node->id)) {
            return Qt::ItemIsSelectable;
        }

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
            if (rights &Collection::CanChangeItem && index.column() == 0) {
                flags = flags | Qt::ItemIsEditable;
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
    return QStringList() << QLatin1String("text/uri-list");
}

bool EntityTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(column);
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
            kWarning() << "Dropped onto item with no parent collection";
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

        if (data->hasFormat(QLatin1String("text/uri-list"))) {

            MimeTypeChecker mimeChecker;
            mimeChecker.setWantedMimeTypes(destCollection.contentMimeTypes());

            const KUrl::List urls = KUrl::List::fromMimeData(data);
            foreach (const KUrl &url, urls) {
                const Collection collection = d->m_collections.value(Collection::fromUrl(url).id());
                if (collection.isValid()) {
                    if (collection.parentCollection().id() == destCollection.id() &&
                        action != Qt::CopyAction) {
                        kDebug() << "Error: source and destination of move are the same.";
                        return false;
                    }

                    if (!mimeChecker.isWantedCollection(collection)) {
                        kDebug() << "unwanted collection" << mimeChecker.wantedMimeTypes() << collection.contentMimeTypes();
                        return false;
                    }

                    if (url.hasQueryItem(QLatin1String("name"))) {
                        const QString collectionName = url.queryItemValue(QLatin1String("name"));
                        const QStringList collectionNames = d->childCollectionNames(destCollection);

                        if (collectionNames.contains(collectionName)) {
                            KMessageBox::error(0, i18n("The target collection '%1' contains already\na collection with name '%2'.",
                                                       destCollection.name(), collection.name()));
                            return false;
                        }
                    }
                } else {
                    const Item item = d->m_items.value(Item::fromUrl(url).id());
                    if (item.isValid()) {
                        if (item.parentCollection().id() == destCollection.id() && action != Qt::CopyAction) {
                            kDebug() << "Error: source and destination of move are the same.";
                            return false;
                        }

                        if (!mimeChecker.isWantedItem(item)) {
                            kDebug() << "unwanted item" << mimeChecker.wantedMimeTypes() << item.mimeType();
                            return false;
                        }
                    }
                }
            }

            KJob *job = PasteHelper::pasteUriList(data, destCollection, action, d->m_session);
            if (!job) {
                return false;
            }

            connect(job, SIGNAL(result(KJob*)), SLOT(pasteJobDone(KJob*)));

            // Accpet the event so that it doesn't propagate.
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

    //TODO: don't use column count here? Use some d-> func.
    if (column >= columnCount() ||
        column < 0) {
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
    } else {
        if (parentNode->id >= 0) {
            childEntities = d->m_childEntities.value(parentNode->id);
        }
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

    if (d->m_collectionFetchStrategy == InvisibleCollectionFetch ||
        d->m_collectionFetchStrategy == FetchNoCollections) {
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

    if (d->m_collectionFetchStrategy == InvisibleCollectionFetch ||
        d->m_collectionFetchStrategy == FetchNoCollections) {
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
    Q_UNUSED(headerGroup);

    return 1;
}

QVariant EntityTreeModel::entityHeaderData(int section, Qt::Orientation orientation, int role, HeaderGroup headerGroup) const
{
    Q_D(const EntityTreeModel);
    // Not needed in this model.
    Q_UNUSED(headerGroup);

    if (section == 0 &&
        orientation == Qt::Horizontal &&
        role == Qt::DisplayRole) {
        if (d->m_rootCollection == Collection::root()) {
            return i18nc("@title:column Name of a thing", "Name");
        }
        return d->m_rootCollection.name();
    }

    return QAbstractItemModel::headerData(section, orientation, role);
}

QVariant EntityTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    const HeaderGroup headerGroup = static_cast<HeaderGroup>((role / static_cast<int>(TerminalUserRole)));

    role %= TerminalUserRole;
    return entityHeaderData(section, orientation, role, headerGroup);
}

QMimeData *EntityTreeModel::mimeData(const QModelIndexList &indexes) const
{
    Q_D(const EntityTreeModel);

    QMimeData *data = new QMimeData();
    KUrl::List urls;
    foreach (const QModelIndex &index, indexes) {
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
            KUrl url = d->m_items.value(node->id).url(Item::Item::UrlWithMimeType);
            // Encode the "virtual" parent
            url.addQueryItem(QLatin1String("parent"), QString::number(node->parent));
            urls << url;
        } else { // if that happens something went horrible wrong
            Q_ASSERT(false);
        }
    }

    urls.populateMimeData(data);

    return data;
}

// Always return false for actions which take place asyncronously, eg via a Job.
bool EntityTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_D(EntityTreeModel);

    const Node *node = reinterpret_cast<Node *>(index.internalPointer());

    if (role == PendingCutRole) {
        if (index.isValid() && value.toBool()) {
            if (Node::Collection == node->type) {
                d->m_pendingCutCollections.append(node->id);
            }

            if (Node::Item == node->type) {
                d->m_pendingCutItems.append(node->id);
            }
        } else {
            d->m_pendingCutCollections.clear();
            d->m_pendingCutItems.clear();
        }
        return true;
    }

    if (index.isValid() &&
        node->type == Node::Collection &&
        (role == CollectionRefRole ||
         role == CollectionDerefRole)) {
        const Collection collection = index.data(CollectionRole).value<Collection>();
        Q_ASSERT(collection.isValid());

        if (role == CollectionDerefRole) {
            d->deref(collection.id());
        } else if (role == CollectionRefRole) {
            d->ref(collection.id());
        }
        return true;
    }

    if (index.column() == 0 &&
        (role &(Qt::EditRole | ItemRole | CollectionRole))) {
        if (Node::Collection == node->type) {

            Collection collection = d->m_collections.value(node->id);

            if (!collection.isValid() || !value.isValid()) {
                return false;
            }

            if (Qt::EditRole == role) {
                collection.setName(value.toString());

                if (collection.hasAttribute<EntityDisplayAttribute>()) {
                    EntityDisplayAttribute *displayAttribute = collection.attribute<EntityDisplayAttribute>();
                    displayAttribute->setDisplayName(value.toString());
                }
            }

            if (Qt::BackgroundRole == role) {
                QColor color = value.value<QColor>();

                if (!color.isValid()) {
                    return false;
                }

                EntityDisplayAttribute *eda = collection.attribute<EntityDisplayAttribute>(Entity::AddIfMissing);
                eda->setBackgroundColor(color);
            }

            if (CollectionRole == role) {
                collection = value.value<Collection>();
            }

            CollectionModifyJob *job = new CollectionModifyJob(collection, d->m_session);
            connect(job, SIGNAL(result(KJob*)),
                    SLOT(updateJobDone(KJob*)));

            return false;
        } else if (Node::Item == node->type) {

            Item item = d->m_items.value(node->id);

            if (!item.isValid() || !value.isValid()) {
                return false;
            }

            if (Qt::EditRole == role) {
                if (item.hasAttribute<EntityDisplayAttribute>()) {
                    EntityDisplayAttribute *displayAttribute = item.attribute<EntityDisplayAttribute>(Entity::AddIfMissing);
                    displayAttribute->setDisplayName(value.toString());
                }
            }

            if (Qt::BackgroundRole == role) {
                QColor color = value.value<QColor>();

                if (!color.isValid()) {
                    return false;
                }

                EntityDisplayAttribute *eda = item.attribute<EntityDisplayAttribute>(Entity::AddIfMissing);
                eda->setBackgroundColor(color);
            }

            if (ItemRole == role) {
                item = value.value<Item>();
                Q_ASSERT(item.id() == node->id);
            }

            ItemModifyJob *itemModifyJob = new ItemModifyJob(item, d->m_session);
            connect(itemModifyJob, SIGNAL(result(KJob*)),
                    SLOT(updateJobDone(KJob*)));

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

    if (d->m_collectionFetchStrategy == InvisibleCollectionFetch ||
        d->m_collectionFetchStrategy == FetchNoCollections) {
        return parent.isValid() ? false : !d->m_items.isEmpty();
    }

    // TODO: Empty collections right now will return true and get a little + to expand.
    // There is probably no way to tell if a collection
    // has child items in akonadi without first attempting an itemFetchJob...
    // Figure out a way to fix this. (Statistics)
    return ((rowCount(parent) > 0) ||
            (canFetchMore(parent) && d->m_itemPopulation == LazyPopulation));
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

bool EntityTreeModel::entityMatch(const Item &item, const QVariant &value, Qt::MatchFlags flags) const
{
    Q_UNUSED(item);
    Q_UNUSED(value);
    Q_UNUSED(flags);
    return false;
}

bool EntityTreeModel::entityMatch(const Collection &collection, const QVariant &value, Qt::MatchFlags flags) const
{
    Q_UNUSED(collection);
    Q_UNUSED(value);
    Q_UNUSED(flags);
    return false;
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

        QModelIndexList list;

        const Collection collection = d->m_collections.value(id);

        if (!collection.isValid()) {
            return list;
        }

        const QModelIndex collectionIndex = d->indexForCollection(collection);
        Q_ASSERT(collectionIndex.isValid());
        list << collectionIndex;

        return list;
    }

    if (role == ItemIdRole || role == ItemRole) {
        Item::Id id;
        if (role == ItemRole) {
            const Item item = value.value<Item>();
            id = item.id();
        } else {
            id = value.toLongLong();
        }
        QModelIndexList list;

        const Item item = d->m_items.value(id);
        if (!item.isValid()) {
            return list;
        }

        return d->indexesForItem(item);
    }

    if (role == EntityUrlRole) {
        const KUrl url(value.toString());
        const Item item = Item::fromUrl(url);

        if (item.isValid()) {
            return d->indexesForItem(d->m_items.value(item.id()));
        }

        const Collection collection = Collection::fromUrl(url);
        QModelIndexList list;
        if (collection.isValid()) {
            list << d->indexForCollection(collection);
        }

        return list;
    }

    if (role != AmazingCompletionRole) {
        return QAbstractItemModel::match(start, role, value, hits, flags);
    }

    // Try to match names, and email addresses.
    QModelIndexList list;

    if (role < 0 ||
        !start.isValid() ||
        !value.isValid()) {
        return list;
    }

    const int column = 0;
    int row = start.row();
    const QModelIndex parentIndex = start.parent();
    const int parentRowCount = rowCount(parentIndex);

    while (row < parentRowCount &&
           (hits == -1 || list.size() < hits)) {
        const QModelIndex idx = index(row, column, parentIndex);
        const Item item = idx.data(ItemRole).value<Item>();

        if (!item.isValid()) {
            const Collection collection = idx.data(CollectionRole).value<Collection>();
            if (!collection.isValid()) {
                continue;
            }

            if (entityMatch(collection, value, flags)) {
                list << idx;
            }

        } else {
            if (entityMatch(item, value, flags)) {
                list << idx;
            }
        }

        ++row;
    }

    return list;
}

bool EntityTreeModel::insertRows(int, int, const QModelIndex &)
{
    return false;
}

bool EntityTreeModel::insertColumns(int, int, const QModelIndex &)
{
    return false;
}

bool EntityTreeModel::removeRows(int, int, const QModelIndex &)
{
    return false;
}

bool EntityTreeModel::removeColumns(int, int, const QModelIndex &)
{
    return false;
}

void EntityTreeModel::setItemPopulationStrategy(ItemPopulationStrategy strategy)
{
    Q_D(EntityTreeModel);
    d->beginResetModel();
    d->m_itemPopulation = strategy;

    if (strategy == NoItemPopulation) {
        disconnect(d->m_monitor, SIGNAL(itemAdded(Akonadi::Item,Akonadi::Collection)),
                   this, SLOT(monitoredItemAdded(Akonadi::Item,Akonadi::Collection)));
        disconnect(d->m_monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)),
                   this, SLOT(monitoredItemChanged(Akonadi::Item,QSet<QByteArray>)));
        disconnect(d->m_monitor, SIGNAL(itemRemoved(Akonadi::Item)),
                   this, SLOT(monitoredItemRemoved(Akonadi::Item)));
        disconnect(d->m_monitor, SIGNAL(itemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)),
                   this, SLOT(monitoredItemMoved(Akonadi::Item,Akonadi::Collection,Akonadi::Collection)));

        disconnect(d->m_monitor, SIGNAL(itemLinked(Akonadi::Item,Akonadi::Collection)),
                   this, SLOT(monitoredItemLinked(Akonadi::Item,Akonadi::Collection)));
        disconnect(d->m_monitor, SIGNAL(itemUnlinked(Akonadi::Item,Akonadi::Collection)),
                   this, SLOT(monitoredItemUnlinked(Akonadi::Item,Akonadi::Collection)));
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

    if (strategy == FetchNoCollections ||
        strategy == InvisibleCollectionFetch) {
        disconnect(d->m_monitor, SIGNAL(collectionChanged(Akonadi::Collection)),
                   this, SLOT(monitoredCollectionChanged(Akonadi::Collection)));
        disconnect(d->m_monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)),
                   this, SLOT(monitoredCollectionAdded(Akonadi::Collection,Akonadi::Collection)));
        disconnect(d->m_monitor, SIGNAL(collectionRemoved(Akonadi::Collection)),
                   this, SLOT(monitoredCollectionRemoved(Akonadi::Collection)));
        disconnect(d->m_monitor,
                   SIGNAL(collectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)),
                   this, SLOT(monitoredCollectionMoved(Akonadi::Collection,Akonadi::Collection,Akonadi::Collection)));
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
    const QAbstractProxyModel *proxy = qobject_cast<const QAbstractProxyModel *>(model);
    const QAbstractItemModel *_model = model;
    while (proxy) {
        proxyChain.prepend(proxy);
        _model = proxy->sourceModel();
        proxy = qobject_cast<const QAbstractProxyModel *>(_model);
    }

    const EntityTreeModel *etm = qobject_cast<const EntityTreeModel *>(_model);
    return qMakePair(proxyChain, etm);
}

static QModelIndex proxiedIndex(const QModelIndex &idx, QList<const QAbstractProxyModel *> proxyChain)
{
    QListIterator<const QAbstractProxyModel *> it(proxyChain);
    QModelIndex _idx = idx;
    while (it.hasNext()) {
        _idx = it.next()->mapFromSource(_idx);
    }
    return _idx;
}

QModelIndex EntityTreeModel::modelIndexForCollection(const QAbstractItemModel *model, const Collection &collection)
{
    QPair<QList<const QAbstractProxyModel *>, const EntityTreeModel *> pair = proxiesAndModel(model);

    Q_ASSERT(pair.second);
    QModelIndex idx = pair.second->d_ptr->indexForCollection(collection);
    return proxiedIndex(idx, pair.first);
}

QModelIndexList EntityTreeModel::modelIndexesForItem(const QAbstractItemModel *model, const Item &item)
{
    QPair<QList<const QAbstractProxyModel *>, const EntityTreeModel *> pair = proxiesAndModel(model);

    if (!pair.second) {
        kWarning() << "Couldn't find an EntityTreeModel";
        return QModelIndexList();
    }

    QModelIndexList list = pair.second->d_ptr->indexesForItem(item);
    QModelIndexList proxyList;
    foreach (const QModelIndex &idx, list) {
        const QModelIndex pIdx = proxiedIndex(idx, pair.first);
        if (pIdx.isValid()) {
            proxyList << pIdx;
        }
    }
    return proxyList;
}

#include "moc_entitytreemodel.cpp"
