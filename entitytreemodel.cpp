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
#include <QtGui/QApplication>
#include <QtGui/QPalette>

#include <KDE/KIcon>
#include <KDE/KLocale>
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

using namespace Akonadi;

EntityTreeModel::EntityTreeModel( Session *session,
                                  ChangeRecorder *monitor,
                                  QObject *parent
                                )
    : QAbstractItemModel( parent ),
    d_ptr( new EntityTreeModelPrivate( this ) )
{
  Q_D( EntityTreeModel );

  d->m_monitor = monitor;
  d->m_session = session;

  d->m_monitor->setChangeRecordingEnabled(false);

  d->m_includeStatistics = true;
  d->m_monitor->fetchCollectionStatistics( true );
  d->m_monitor->collectionFetchScope().setAncestorRetrieval( Akonadi::CollectionFetchScope::All );

  d->m_mimeChecker.setWantedMimeTypes( d->m_monitor->mimeTypesMonitored() );

  connect( monitor, SIGNAL( mimeTypeMonitored( const QString&, bool ) ),
           SLOT( monitoredMimeTypeChanged( const QString&, bool ) ) );

  // monitor collection changes
  connect( monitor, SIGNAL( collectionChanged( const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionChanged( const Akonadi::Collection& ) ) );
  connect( monitor, SIGNAL( collectionAdded( const Akonadi::Collection&, const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionAdded( const Akonadi::Collection&, const Akonadi::Collection& ) ) );
  connect( monitor, SIGNAL( collectionRemoved( const Akonadi::Collection& ) ),
           SLOT( monitoredCollectionRemoved( const Akonadi::Collection&) ) );
  connect( monitor,
           SIGNAL( collectionMoved( const Akonadi::Collection &, const Akonadi::Collection &, const Akonadi::Collection & ) ),
           SLOT( monitoredCollectionMoved( const Akonadi::Collection &, const Akonadi::Collection &, const Akonadi::Collection & ) ) );

  if ( !monitor->itemFetchScope().isEmpty() )
  {
    // Monitor item changes.
    connect( monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
            SLOT( monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
    connect( monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
            SLOT( monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );
    connect( monitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
            SLOT( monitoredItemRemoved( const Akonadi::Item& ) ) );
    connect( monitor, SIGNAL( itemMoved( const Akonadi::Item, const Akonadi::Collection, const Akonadi::Collection ) ),
            SLOT( monitoredItemMoved( const Akonadi::Item, const Akonadi::Collection, const Akonadi::Collection ) ) );

    connect( monitor, SIGNAL( itemLinked( const Akonadi::Item&, const Akonadi::Collection& )),
            SLOT( monitoredItemLinked( const Akonadi::Item&, const Akonadi::Collection& )));
    connect( monitor, SIGNAL( itemUnlinked( const Akonadi::Item&, const Akonadi::Collection& )),
            SLOT( monitoredItemUnlinked( const Akonadi::Item&, const Akonadi::Collection& )));
  }
  connect( monitor, SIGNAL( collectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics& ) ),
           SLOT(monitoredCollectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics& ) ) );

//   connect( q, SIGNAL( modelReset() ), q, SLOT( slotModelReset() ) );

  d->m_rootCollection = Collection::root();
  d->m_rootCollectionDisplayName = QLatin1String( "[*]" );

  // Initializes the model cleanly.
  clearAndReset();
}

EntityTreeModel::~EntityTreeModel()
{
  Q_D( EntityTreeModel );

  foreach( QList<Node*> list, d->m_childEntities ) {
    qDeleteAll(list);
    list.clear();
  }

  delete d_ptr;
}

bool EntityTreeModel::systemEntitiesShown() const
{
  Q_D( const EntityTreeModel );
  return d->m_showSystemEntities;
}

void EntityTreeModel::setShowSystemEntities( bool show )
{
  Q_D( EntityTreeModel );
  d->m_showSystemEntities = show;
}

void EntityTreeModel::clearAndReset()
{
  Q_D( EntityTreeModel );
  d->m_collections.clear();
  d->m_items.clear();
  d->m_childEntities.clear();
  reset();
  QTimer::singleShot( 0, this, SLOT( startFirstListJob() ) );
}

Collection EntityTreeModel::collectionForId( Collection::Id id ) const
{
  Q_D( const EntityTreeModel );
  return d->m_collections.value( id );
}

Item EntityTreeModel::itemForId( Item::Id id ) const
{
  Q_D( const EntityTreeModel );
  return d->m_items.value( id );
}

int EntityTreeModel::columnCount( const QModelIndex & parent ) const
{
// TODO: Statistics?
  if ( parent.isValid() && parent.column() != 0 )
    return 0;

  return qMax( entityColumnCount( CollectionTreeHeaders ), entityColumnCount( ItemListHeaders ) );
}


QVariant EntityTreeModel::entityData( const Item &item, int column, int role ) const
{
  if ( column == 0 ) {
    switch ( role ) {
      case Qt::DisplayRole:
      case Qt::EditRole:
        if ( item.hasAttribute<EntityDisplayAttribute>() &&
             !item.attribute<EntityDisplayAttribute>()->displayName().isEmpty() ) {
          return item.attribute<EntityDisplayAttribute>()->displayName();
        } else {
          return item.remoteId();
        }
        break;
      case Qt::DecorationRole:
        if ( item.hasAttribute<EntityDisplayAttribute>() &&
             !item.attribute<EntityDisplayAttribute>()->iconName().isEmpty() )
          return item.attribute<EntityDisplayAttribute>()->icon();
        break;
      default:
        break;
    }
  }

  return QVariant();
}

QVariant EntityTreeModel::entityData( const Collection &collection, int column, int role ) const
{
  Q_D(const EntityTreeModel);

  if ( column > 0 )
    return QString();

  if ( collection == Collection::root() ) {
    // Only display the root collection. It may not be edited.
    if ( role == Qt::DisplayRole )
      return d->m_rootCollectionDisplayName;

    if ( role == Qt::EditRole )
      return QVariant();
  }

  if ( column == 0 && (role == Qt::DisplayRole || role == Qt::EditRole) ) {
    if ( collection.hasAttribute<EntityDisplayAttribute>() &&
          !collection.attribute<EntityDisplayAttribute>()->displayName().isEmpty() )
      return collection.attribute<EntityDisplayAttribute>()->displayName();
    return collection.name();
  }

  switch ( role ) {
    case Qt::DisplayRole:
    case Qt::EditRole:
      if ( column == 0 ) {
        if ( collection.hasAttribute<EntityDisplayAttribute>() &&
             !collection.attribute<EntityDisplayAttribute>()->displayName().isEmpty() ) {
          return collection.attribute<EntityDisplayAttribute>()->displayName();
        }
        return collection.name();
      }
      break;
    case Qt::DecorationRole:
      if ( collection.hasAttribute<EntityDisplayAttribute>() &&
           !collection.attribute<EntityDisplayAttribute>()->iconName().isEmpty() ) {
        return collection.attribute<EntityDisplayAttribute>()->icon();
      }
      return KIcon( CollectionUtils::defaultIconName( collection ) );
    default:
      break;
  }

  return QVariant();
}

QVariant EntityTreeModel::data( const QModelIndex & index, int role ) const
{
  Q_D( const EntityTreeModel );
  if ( role == SessionRole )
    return QVariant::fromValue( qobject_cast<QObject *>( d->m_session ) );

  // Ugly, but at least the API is clean.
  const HeaderGroup headerGroup = static_cast<HeaderGroup>( ( role / static_cast<int>( TerminalUserRole ) ) );

  role %= TerminalUserRole;
  if ( !index.isValid() )
  {
    if ( ColumnCountRole != role )
      return QVariant();
    return entityColumnCount( headerGroup );
  }

  if ( ColumnCountRole == role )
    return entityColumnCount( headerGroup );

  const Node *node = reinterpret_cast<Node *>( index.internalPointer() );

  if ( ParentCollectionRole == role )
  {
    const Collection parentCollection = d->m_collections.value( node->parent );
    Q_ASSERT(parentCollection.isValid());

    return QVariant::fromValue(parentCollection);
  }

  if ( Node::Collection == node->type ) {

    if ( role == Qt::ForegroundRole && d->m_pendingCutCollections.contains( node->id ) )
    {
       return QApplication::palette().color( QPalette::Disabled, QPalette::Text);
    }

    const Collection collection = d->m_collections.value( node->id );

    if ( !collection.isValid() )
      return QVariant();

    switch ( role )
    {
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
      return QVariant::fromValue( collection );
    default:
      return entityData( collection, index.column(), role );
    }

  } else if ( Node::Item == node->type ) {
    if ( role == Qt::ForegroundRole && d->m_pendingCutItems.contains( node->id ) )
    {
       return QApplication::palette().color( QPalette::Disabled, QPalette::Text );
    }

    const Item item = d->m_items.value( node->id );
    if ( !item.isValid() )
      return QVariant();

    switch ( role )
    {
    case MimeTypeRole:
      return item.mimeType();
    case RemoteIdRole:
      return item.remoteId();
    case ItemRole:
      return QVariant::fromValue( item );
    case ItemIdRole:
      return item.id();
    case CollectionIdRole:
      return -1;
    case LoadedPartsRole:
      return QVariant::fromValue( item.loadedPayloadParts() );
    case AvailablePartsRole:
      return QVariant::fromValue( item.availablePayloadParts() );
    default:
      return entityData( item, index.column(), role );
    }
  }
  return QVariant();
}


Qt::ItemFlags EntityTreeModel::flags( const QModelIndex & index ) const
{
  Q_D( const EntityTreeModel );
  // Pass modeltest.
  // http://labs.trolltech.com/forums/topic/79
  if ( !index.isValid() )
    return 0;

  Qt::ItemFlags flags = QAbstractItemModel::flags( index );

  const Node *node = reinterpret_cast<Node *>(index.internalPointer());

  if ( Node::Collection == node->type ) {
    const Collection collection = d->m_collections.value( node->id );
    if ( collection.isValid() ) {

      if ( collection == Collection::root() ) {
        // Selectable and displayable only.
        return flags;
      }

      const int rights = collection.rights();

      if ( rights & Collection::CanChangeCollection ) {
        if ( index.column() == 0 )
          flags |= Qt::ItemIsEditable;
        // Changing the collection includes changing the metadata (child entityordering).
        // Need to allow this by drag and drop.
        flags |= Qt::ItemIsDropEnabled;
      }
      if ( rights & ( Collection::CanCreateCollection | Collection::CanCreateItem | Collection::CanLinkItem ) ) {
        // Can we drop new collections and items into this collection?
        flags |= Qt::ItemIsDropEnabled;
      }

      // dragging is always possible, even for read-only objects, but they can only be copied, not moved.
      flags |= Qt::ItemIsDragEnabled;

    }
  } else if ( Node::Item == node->type ) {

    // Rights come from the parent collection.
    const Node *parentNode = reinterpret_cast<Node *>( index.parent().internalPointer() );

    // TODO: Is this right for the root collection? I think so, but only by chance.
    // But will it work if m_rootCollection is different from Collection::root?
    // Should probably rely on index.parent().isValid() for that.
    const Collection parentCollection = d->m_collections.value( parentNode->id );
    if ( parentCollection.isValid() ) {
      const int rights = parentCollection.rights();

      // Can't drop onto items.
      if ( rights & Collection::CanChangeItem && index.column() == 0 ) {
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
  return Qt::CopyAction | Qt::MoveAction | Qt::LinkAction;
}

QStringList EntityTreeModel::mimeTypes() const
{
  // TODO: Should this return the mimetypes that the items provide? Allow dragging a contact from here for example.
  return QStringList() << QLatin1String( "text/uri-list" );
}

bool EntityTreeModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_UNUSED( row );
  Q_UNUSED( column );
  Q_D( EntityTreeModel );

  // Can't drop onto Collection::root.
  if ( !parent.isValid() )
    return false;

  // TODO Use action and collection rights and return false if necessary

  // if row and column are -1, then the drop was on parent directly.
  // data should then be appended on the end of the items of the collections as appropriate.
  // That will mean begin insert rows etc.
  // Otherwise it was a sibling of the row^th item of parent.
  // Needs to be handled when ordering is accounted for.

  // Handle dropping between items as well as on items.
//   if (row != -1 && column != -1)
//   {
//   }


  if ( action == Qt::IgnoreAction )
    return true;

// Shouldn't do this. Need to be able to drop vcards for example.
//   if (!data->hasFormat("text/uri-list"))
//       return false;

  Node *node = reinterpret_cast<Node *>( parent.internalId() );

  Q_ASSERT( node );

  if ( Node::Item == node->type ) {
    if ( !parent.parent().isValid() )
    {
      // The drop is somehow on an item with no parent (shouldn't happen)
      // The drop should be considered handled anyway.
      kWarning() << "Dropped onto item with no parent collection";
      return true;
    }

    // A drop onto an item should be considered as a drop onto its parent collection
    node = reinterpret_cast<Node *>( parent.parent().internalId() );
  }

  if ( Node::Collection == node->type ) {
    const Collection destCollection = d->m_collections.value( node->id );

    // Applications can't create new collections in root. Only resources can.
    if ( destCollection == Collection::root() )
      // Accept the event so that it doesn't propagate.
      return true;

    if ( data->hasFormat( QLatin1String( "text/uri-list" ) ) ) {

      MimeTypeChecker mimeChecker;
      mimeChecker.setWantedMimeTypes( destCollection.contentMimeTypes() );

      const KUrl::List urls = KUrl::List::fromMimeData( data );
      foreach ( const KUrl &url, urls ) {
        const Collection collection = d->m_collections.value( Collection::fromUrl( url ).id() );
        if ( collection.isValid() )
        {
          if ( collection.parentCollection().id() == destCollection.id() )
          {
            kDebug() << "Error: source and destination of move are the same.";
            return false;
          }

          if ( !mimeChecker.isWantedCollection( collection ) )
          {
            kDebug() << "unwanted collection" << mimeChecker.wantedMimeTypes() << collection.contentMimeTypes();
            return false;
          }
        } else {
          const Item item = d->m_items.value( Item::fromUrl( url ).id() );
          if ( item.isValid() )
          {
            if ( item.parentCollection().id() == destCollection.id() )
            {
              kDebug() << "Error: source and destination of move are the same.";
              return false;
            }

            if ( !mimeChecker.isWantedItem( item ) )
            {
              kDebug() << "unwanted item" << mimeChecker.wantedMimeTypes() << item.mimeType();
              return false;
            }
          }
        }
      }

      KJob *job = PasteHelper::pasteUriList( data, destCollection, action, d->m_session );
      if (!job)
        return false;

      connect( job, SIGNAL(result(KJob*)), SLOT(pasteJobDone(KJob*)) );

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

QModelIndex EntityTreeModel::index( int row, int column, const QModelIndex & parent ) const
{

  Q_D( const EntityTreeModel );

  if ( parent.column() > 0 )
  {
    return QModelIndex();
  }

  //TODO: don't use column count here? Use some d-> func.
  if ( column >= columnCount() || column < 0 )
    return QModelIndex();

  QList<Node*> childEntities;

  const Node *parentNode = reinterpret_cast<Node*>( parent.internalPointer() );

  if ( !parentNode || !parent.isValid() ) {
    if ( d->m_showRootCollection )
      childEntities << d->m_childEntities.value( -1 );
    else
      childEntities = d->m_childEntities.value( d->m_rootCollection.id() );
  } else {
    if ( parentNode->id >= 0 )
      childEntities = d->m_childEntities.value( parentNode->id );
  }

  const int size = childEntities.size();
  if ( row < 0 || row >= size )
    return QModelIndex();

  Node *node = childEntities.at( row );

  return createIndex( row, column, reinterpret_cast<void*>( node ) );
}

QModelIndex EntityTreeModel::parent( const QModelIndex & index ) const
{
  Q_D( const EntityTreeModel );

  if ( !index.isValid() )
    return QModelIndex();

  const Node *node = reinterpret_cast<Node*>( index.internalPointer() );

  if ( !node )
    return QModelIndex();

  const Collection collection = d->m_collections.value( node->parent );

  if ( !collection.isValid() )
    return QModelIndex();

  if ( collection.id() == d->m_rootCollection.id() ) {
    if ( !d->m_showRootCollection )
      return QModelIndex();
    else
      return createIndex( 0, 0, reinterpret_cast<void *>( d->m_rootNode ) );
  }

  const int row = d->indexOf( d->m_childEntities.value( collection.parentCollection().id() ), collection.id() );

  Q_ASSERT( row >= 0 );
  Node *parentNode = d->m_childEntities.value( collection.parentCollection().id() ).at( row );

  return createIndex( row, 0, reinterpret_cast<void*>( parentNode ) );
}

int EntityTreeModel::rowCount( const QModelIndex & parent ) const
{
  Q_D( const EntityTreeModel );

  const Node *node = reinterpret_cast<Node*>( parent.internalPointer() );

  qint64 id;
  if ( !parent.isValid() ) {
    // If we're showing the root collection then it will be the only child of the root.
    if ( d->m_showRootCollection )
      return d->m_childEntities.value( -1 ).size();

    id = d->m_rootCollection.id();
  } else {

    if ( !node )
      return 0;

    if ( Node::Item == node->type )
      return 0;

    id = node->id;
  }

  if ( parent.column() <= 0 )
    return d->m_childEntities.value( id ).size();

  return 0;
}

int EntityTreeModel::entityColumnCount( HeaderGroup headerGroup ) const
{
  // Not needed in this model.
  Q_UNUSED(headerGroup);

  return 1;
}

QVariant EntityTreeModel::entityHeaderData( int section, Qt::Orientation orientation, int role, int headerSet) const
{
  // Not needed in this model.
  Q_UNUSED(headerSet);

  if ( section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole )
    return i18nc( "@title:column, name of a thing", "Name" );

  return QAbstractItemModel::headerData( section, orientation, role );
}

QVariant EntityTreeModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  const int headerSet = (role / TerminalUserRole);

  role %= TerminalUserRole;
  return entityHeaderData( section, orientation, role, headerSet );
}

QMimeData *EntityTreeModel::mimeData( const QModelIndexList &indexes ) const
{
  Q_D( const EntityTreeModel );

  QMimeData *data = new QMimeData();
  KUrl::List urls;
  foreach( const QModelIndex &index, indexes ) {
    if ( index.column() != 0 )
      continue;

    if (!index.isValid())
      continue;

    const Node *node = reinterpret_cast<Node*>( index.internalPointer() );

    if ( Node::Collection == node->type )
      urls << d->m_collections.value(node->id).url();
    else if ( Node::Item == node->type )
      urls << d->m_items.value( node->id ).url( Item::UrlWithMimeType );
    else // if that happens something went horrible wrong
      Q_ASSERT(false);
  }

  urls.populateMimeData( data );

  return data;
}

// Always return false for actions which take place asyncronously, eg via a Job.
bool EntityTreeModel::setData( const QModelIndex &index, const QVariant &value, int role )
{
  Q_D( EntityTreeModel );

  const Node *node = reinterpret_cast<Node*>( index.internalPointer() );

  if ( role == PendingCutRole )
  {
    if ( index.isValid() && value.toBool() )
    {
      if ( Node::Collection == node->type )
        d->m_pendingCutCollections.append( node->id );

      if ( Node::Item == node->type )
        d->m_pendingCutItems.append( node->id );
    }
    else
    {
      d->m_pendingCutCollections.clear();
      d->m_pendingCutItems.clear();
    }
    return true;
  }

  if ( index.isValid() && node->type == Node::Collection && ( role == CollectionRefRole || role == CollectionDerefRole ) )
  {
    Collection col = index.data( CollectionRole ).value<Collection>();
    Q_ASSERT( col.isValid() );

    if ( role == CollectionDerefRole )
      d->deref( col.id() );
    else if ( role == CollectionRefRole )
      d->ref( col.id() );
  }

  if ( index.column() == 0 && (role & ( Qt::EditRole | ItemRole | CollectionRole ) ) ) {
    if ( Node::Collection == node->type ) {

      Collection collection = d->m_collections.value( node->id );

      if ( !collection.isValid() || !value.isValid() )
        return false;

      if ( Qt::EditRole == role ) {
        collection.setName( value.toString() );

        if ( collection.hasAttribute<EntityDisplayAttribute>() ) {
          EntityDisplayAttribute *displayAttribute = collection.attribute<EntityDisplayAttribute>();
          displayAttribute->setDisplayName( value.toString() );
          collection.addAttribute( displayAttribute );
        }
      }

      if ( CollectionRole == role )
        collection = value.value<Collection>();

      CollectionModifyJob *job = new CollectionModifyJob( collection, d->m_session );
      connect( job, SIGNAL( result( KJob* ) ),
               SLOT( updateJobDone( KJob* ) ) );

      return false;
    } else if (Node::Item == node->type) {

      Item item = d->m_items.value( node->id );

      if ( !item.isValid() || !value.isValid() )
        return false;

      if ( Qt::EditRole == role ) {
        if ( item.hasAttribute<EntityDisplayAttribute>() ) {
          EntityDisplayAttribute *displayAttribute = item.attribute<EntityDisplayAttribute>( Entity::AddIfMissing );
          displayAttribute->setDisplayName( value.toString() );
          item.addAttribute( displayAttribute );
        }
      }

      if ( ItemRole == role )
        item = value.value<Item>();

      ItemModifyJob *itemModifyJob = new ItemModifyJob( item, d->m_session );
      connect( itemModifyJob, SIGNAL( result( KJob* ) ),
               SLOT( updateJobDone( KJob* ) ) );

      return false;
    }
  }

  return QAbstractItemModel::setData( index, value, role );
}

bool EntityTreeModel::canFetchMore( const QModelIndex & parent ) const
{
  Q_D(const EntityTreeModel);
  const Item item = parent.data( ItemRole ).value<Item>();

  if ( item.isValid() ) {
    // items can't have more rows.
    // TODO: Should I use this for fetching more of an item, ie more payload parts?
    return false;
  } else {
    // but collections can...
    const Collection::Id colId = parent.data( CollectionIdRole ).toULongLong();

    // But the root collection can't...
    if ( Collection::root().id() == colId )
    {
      return false;
    }

    foreach (Node *node, d->m_childEntities.value( colId ) )
    {
      if ( Node::Item == node->type )
      {
        // Only try to fetch more from a collection if we don't already have items in it.
        // Otherwise we'd spend all the time listing items in collections.
        // This means that collections which don't contain items get a lot of item fetch jobs started on them.
        // Will fix that later.
        return false;
      }
    }
    return true;
  }

  // TODO: It might be possible to get akonadi to tell us if a collection is empty
  //       or not and use that information instead of assuming all collections are not empty.
  //       Using Collection statistics?
}

void EntityTreeModel::fetchMore( const QModelIndex & parent )
{
  Q_D( EntityTreeModel );

  if (!canFetchMore(parent))
    return;

  if ( d->m_itemPopulation == ImmediatePopulation )
    // Nothing to do. The items are already in the model.
    return;
  else if ( d->m_itemPopulation == LazyPopulation ) {
    const Collection collection = parent.data( CollectionRole ).value<Collection>();

    if ( !collection.isValid() )
      return;

    d->fetchItems( collection );
  }
}

bool EntityTreeModel::hasChildren( const QModelIndex &parent ) const
{
  Q_D( const EntityTreeModel );
  // TODO: Empty collections right now will return true and get a little + to expand.
  // There is probably no way to tell if a collection
  // has child items in akonadi without first attempting an itemFetchJob...
  // Figure out a way to fix this. (Statistics)
  return ((rowCount(parent) > 0) || (canFetchMore( parent ) && d->m_itemPopulation == LazyPopulation));
}

bool EntityTreeModel::match(const Item &item, const QVariant &value, Qt::MatchFlags flags) const
{
  Q_UNUSED(item);
  Q_UNUSED(value);
  Q_UNUSED(flags);
  return false;
}

bool EntityTreeModel::match(const Collection &collection, const QVariant &value, Qt::MatchFlags flags) const
{
  Q_UNUSED(collection);
  Q_UNUSED(value);
  Q_UNUSED(flags);
  return false;
}

QModelIndexList EntityTreeModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags ) const
{
  if (role != AmazingCompletionRole)
    return QAbstractItemModel::match(start, role, value, hits, flags);

  // Try to match names, and email addresses.
  QModelIndexList list;

  if (role < 0 || !start.isValid() || !value.isValid())
    return list;

  const int column = 0;
  int row = start.row();
  QModelIndex parentIdx = start.parent();
  int parentRowCount = rowCount(parentIdx);

  while (row < parentRowCount && (hits == -1 || list.size() < hits))
  {
    QModelIndex idx = index(row, column, parentIdx);
    Item item = idx.data(ItemRole).value<Item>();
    if (!item.isValid())
    {
      Collection col = idx.data(CollectionRole).value<Collection>();
      if (!col.isValid())
      {
        continue;
      }
      if (match(col, value, flags))
        list << idx;
    } else {
      if (match(item, value, flags))
      {
        list << idx;
      }
    }
    ++row;
  }
  return list;

}

bool EntityTreeModel::insertRows( int, int, const QModelIndex& )
{
  return false;
}

bool EntityTreeModel::insertColumns( int, int, const QModelIndex& )
{
  return false;
}

bool EntityTreeModel::removeRows( int, int, const QModelIndex& )
{
  return false;
}

bool EntityTreeModel::removeColumns( int, int, const QModelIndex& )
{
  return false;
}

QModelIndex EntityTreeModel::indexForCollection( const Collection &collection ) const
{
  Q_D(const EntityTreeModel);

  // The id of the parent of Collection::root is not guaranteed to be -1 as assumed by startFirstListJob,
  // we ensure that we use -1 for the invalid Collection.
  const Collection::Id parentId = collection.parentCollection().isValid() ? collection.parentCollection().id() : -1;

  const int row = d->indexOf( d->m_childEntities.value( parentId ), collection.id() );

  if ( row < 0 )
    return QModelIndex();

  Node *node = d->m_childEntities.value( parentId ).at( row );

  return createIndex( row, 0, reinterpret_cast<void*>( node ) );
}

QModelIndexList EntityTreeModel::indexesForItem( const Item &item ) const
{
  Q_D(const EntityTreeModel);
  QModelIndexList indexes;

  const Collection::List collections = d->getParentCollections( item );
  const qint64 id = item.id();

  foreach ( const Collection &collection, collections ) {
    const int row = d->indexOf( d->m_childEntities.value( collection.id() ), id );

    Node *node = d->m_childEntities.value( collection.id() ).at( row );

    indexes << createIndex( row, 0, reinterpret_cast<void*>( node ) );
  }

  return indexes;
}

void EntityTreeModel::setItemPopulationStrategy( ItemPopulationStrategy strategy )
{
  Q_D( EntityTreeModel );
  d->m_itemPopulation = strategy;

  if ( strategy == NoItemPopulation )
  {
    disconnect( d->m_monitor, SIGNAL( itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ),
            this, SLOT( monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& ) ) );
    disconnect( d->m_monitor, SIGNAL( itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ),
            this, SLOT( monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) ) );
    disconnect( d->m_monitor, SIGNAL( itemRemoved( const Akonadi::Item& ) ),
            this, SLOT( monitoredItemRemoved( const Akonadi::Item& ) ) );
    disconnect( d->m_monitor, SIGNAL( itemMoved( const Akonadi::Item, const Akonadi::Collection, const Akonadi::Collection ) ),
            this, SLOT( monitoredItemMoved( const Akonadi::Item, const Akonadi::Collection, const Akonadi::Collection ) ) );

    disconnect( d->m_monitor, SIGNAL( itemLinked( const Akonadi::Item&, const Akonadi::Collection& )),
            this, SLOT( monitoredItemLinked( const Akonadi::Item&, const Akonadi::Collection& )));
    disconnect( d->m_monitor, SIGNAL( itemUnlinked( const Akonadi::Item&, const Akonadi::Collection& )),
            this, SLOT( monitoredItemUnlinked( const Akonadi::Item&, const Akonadi::Collection& )));
  }

  d->m_monitor->d_ptr->useRefCounting = ( strategy == LazyPopulation );

  clearAndReset();
}

EntityTreeModel::ItemPopulationStrategy EntityTreeModel::itemPopulationStrategy() const
{
  Q_D(const EntityTreeModel);
  return d->m_itemPopulation;
}

void EntityTreeModel::setIncludeRootCollection( bool include )
{
  Q_D(EntityTreeModel);
  d->m_showRootCollection = include;
  clearAndReset();
}

bool EntityTreeModel::includeRootCollection() const
{
  Q_D(const EntityTreeModel);
  return d->m_showRootCollection;
}

void EntityTreeModel::setRootCollectionDisplayName( const QString &displayName )
{
  Q_D(EntityTreeModel);
  d->m_rootCollectionDisplayName = displayName;

  // TODO: Emit datachanged if it is being shown.
}

QString EntityTreeModel::rootCollectionDisplayName() const
{
  Q_D( const EntityTreeModel);
  return d->m_rootCollectionDisplayName;
}

void EntityTreeModel::setCollectionFetchStrategy( CollectionFetchStrategy strategy )
{
  Q_D( EntityTreeModel);
  d->m_collectionFetchStrategy = strategy;
  clearAndReset();
}

EntityTreeModel::CollectionFetchStrategy EntityTreeModel::collectionFetchStrategy() const
{
  Q_D( const EntityTreeModel);
  return d->m_collectionFetchStrategy;
}

#include "entitytreemodel.moc"
