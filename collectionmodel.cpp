/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collectionmodel.h"

#include "collection.h"
#include "collectionstatusjob.h"
#include "collectionlistjob.h"
#include "collectionmodifyjob.h"
#include "itemstorejob.h"
#include "itemappendjob.h"
#include "monitor.h"
#include "session.h"

#include <kdebug.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <klocale.h>
#include <kurl.h>

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QMimeData>
#include <QtCore/QQueue>
#include <QtCore/QEventLoop>
#include <QtGui/QPixmap>

using namespace Akonadi;

class CollectionModel::Private
{
  public:
    Private( CollectionModel *parent )
      : mParent( parent ), fetchStatus( false )
    {
    }

    CollectionModel *mParent;
    QHash<int, Collection> collections;
    QHash<int, QList<int> > childCollections;
    Monitor *monitor;
    Session *session;
    QStringList mimeTypes;
    bool fetchStatus;

    void collectionRemoved( int );
    void collectionChanged( const Akonadi::Collection& );
    void updateDone( KJob* );
    void collectionStatusChanged( int, const Akonadi::CollectionStatus& );
    void listDone( KJob* );
    void editDone( KJob* );
    void appendDone( KJob* );
    void collectionsChanged( const Akonadi::Collection::List &cols );

    void updateSupportedMimeTypes( Collection col )
    {
      QStringList l = col.contentTypes();
      for ( QStringList::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it ) {
        if ( (*it) == Collection::collectionMimeType() )
          continue;
        if ( !mimeTypes.contains( *it  ) )
          mimeTypes << *it;
      }
    }
};

void CollectionModel::Private::collectionRemoved( int collection )
{
  QModelIndex colIndex = mParent->indexForId( collection );
  if ( colIndex.isValid() ) {
    QModelIndex parentIndex = mParent->parent( colIndex );
    // collection is still somewhere in the hierarchy
    mParent->removeRowFromModel( colIndex.row(), parentIndex );
  } else {
    if ( collections.contains( collection ) ) {
      // collection is orphan, ie. the parent has been removed already
      collections.remove( collection );
      childCollections.remove( collection );
    }
  }
}

void CollectionModel::Private::collectionChanged( const Akonadi::Collection &collection )
{
  // What kind of change is it ?
  int oldParentId = collections.value( collection.id() ).parent();
  int newParentId = collection.parent();
  if ( newParentId !=  oldParentId && oldParentId >= 0 ) { // It's a move
    mParent->removeRowFromModel( mParent->indexForId( collections[ collection.id() ].id() ).row(), mParent->indexForId( oldParentId ) );
    Collection newParent;
    if ( newParentId == Collection::root().id() )
      newParent = Collection::root();
    else
      newParent = collections.value( newParentId );
    CollectionListJob *job = new CollectionListJob( newParent, CollectionListJob::Recursive, session );
    mParent->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                      mParent, SLOT(collectionsChanged(Akonadi::Collection::List)) );
    mParent->connect( job, SIGNAL( result( KJob* ) ),
                    mParent, SLOT( listDone( KJob* ) ) );

  }
  else { // It's a simple change
    CollectionListJob *job = new CollectionListJob( collection, CollectionListJob::Local, session );
    mParent->connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)),
                      mParent, SLOT(collectionsChanged(Akonadi::Collection::List)) );
    mParent->connect( job, SIGNAL( result( KJob* ) ),
                    mParent, SLOT( listDone( KJob* ) ) );
  }

}

void CollectionModel::Private::updateDone( KJob *job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning( 5250 ) << "Job error:" << job->errorString();
  } else {
    CollectionStatusJob *csjob = static_cast<CollectionStatusJob*>( job );
    Collection result = csjob->collection();
    collectionStatusChanged( result.id(), csjob->status() );
  }
}

void CollectionModel::Private::collectionStatusChanged( int collection, const Akonadi::CollectionStatus & status )
{
  if ( !collections.contains( collection ) )
    kWarning( 5250 ) << "Got status response for non-existing collection:" << collection;
  else {
    collections[ collection ].setStatus( status );

    Collection col = collections.value( collection );
    QModelIndex startIndex = mParent->indexForId( col.id() );
    QModelIndex endIndex = mParent->indexForId( col.id(), mParent->columnCount( mParent->parent( startIndex ) ) - 1 );
    emit mParent->dataChanged( startIndex, endIndex );
  }
}

void CollectionModel::Private::listDone( KJob *job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Job error: " << job->errorString() << endl;
  }
}

void CollectionModel::Private::editDone( KJob * job )
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Edit failed: " << job->errorString();
  }
}

void CollectionModel::Private::appendDone(KJob * job)
{
  if ( job->error() ) {
    kWarning( 5250 ) << "Append failed:" << job->errorString();
    // TODO: error handling
  }
}

void CollectionModel::Private::collectionsChanged( const Collection::List &cols )
{
  foreach( Collection col, cols ) {
    if ( collections.contains( col.id() ) ) {
      // collection already known
      col.setStatus( collections.value( col.id() ).status() );
      collections[ col.id() ] = col;
      QModelIndex startIndex = mParent->indexForId( col.id() );
      QModelIndex endIndex = mParent->indexForId( col.id(), mParent->columnCount( mParent->parent( startIndex ) ) - 1 );
      emit mParent->dataChanged( startIndex, endIndex );
      continue;
    }
    collections.insert( col.id(), col );
    QModelIndex parentIndex = mParent->indexForId( col.parent() );
    if ( parentIndex.isValid() || col.parent() == Collection::root().id() ) {
      mParent->beginInsertRows( parentIndex, mParent->rowCount( parentIndex ), mParent->rowCount( parentIndex ) );
      childCollections[ col.parent() ].append( col.id() );
      mParent->endInsertRows();
    } else {
      childCollections[ col.parent() ].append( col.id() );
    }

    updateSupportedMimeTypes( col );

    // start a status job for every collection to get message counts, etc.
    if ( fetchStatus && col.type() != Collection::VirtualParent ) {
      CollectionStatusJob* csjob = new CollectionStatusJob( col, session );
      mParent->connect( csjob, SIGNAL(result(KJob*)), mParent, SLOT(updateDone(KJob*)) );
    }
  }
}


CollectionModel::CollectionModel( QObject * parent ) :
    QAbstractItemModel( parent ),
    d( new Private( this ) )
{
  d->session = new Session( QByteArray("CollectionModel-") + QByteArray::number( qrand() ), this );

  // start a list job
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive, d->session );
  connect( job, SIGNAL(collectionsReceived(Akonadi::Collection::List)), SLOT(collectionsChanged(Akonadi::Collection::List)) );
  connect( job, SIGNAL(result(KJob*)), SLOT(listDone(KJob*)) );

  // monitor collection changes
  d->monitor = new Monitor();
  d->monitor->monitorCollection( Collection::root() );
  d->monitor->fetchCollection( true );
  connect( d->monitor, SIGNAL(collectionChanged(const Akonadi::Collection&)), SLOT(collectionChanged(const Akonadi::Collection&)) );
  connect( d->monitor, SIGNAL(collectionAdded(Akonadi::Collection,Akonadi::Collection)), SLOT(collectionChanged(Akonadi::Collection)) );
  connect( d->monitor, SIGNAL(collectionRemoved(int,QString)), SLOT(collectionRemoved(int)) );
  connect( d->monitor, SIGNAL(collectionStatusChanged(int,Akonadi::CollectionStatus)),
           SLOT(collectionStatusChanged(int,Akonadi::CollectionStatus)) );

  // ### Hack to get the kmail resource folder icons
  KIconLoader::global()->addAppDir( QLatin1String( "kmail" ) );
}

CollectionModel::~CollectionModel()
{
  d->childCollections.clear();
  d->collections.clear();

  delete d->monitor;
  d->monitor = 0;

  delete d;
}

int CollectionModel::columnCount( const QModelIndex & parent ) const
{
  if (parent.isValid() && parent.column() != 0)
    return 0;
  return 1;
}

QVariant CollectionModel::data( const QModelIndex & index, int role ) const
{
  if ( !index.isValid() )
    return QVariant();

  Collection col = d->collections.value( index.internalId() );
  if ( !col.isValid() )
    return QVariant();

  if ( index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole) ) {
    return col.name();
  }

  switch ( role ) {
    case Qt::DecorationRole:
      if ( index.column() == 0 ) {
        if ( col.type() == Collection::Resource )
          return SmallIcon( QLatin1String( "network-wired" ) );
        if ( col.type() == Collection::VirtualParent )
          return SmallIcon( QLatin1String( "edit-find" ) );
        if ( col.type() == Collection::Virtual )
          return SmallIcon( QLatin1String( "folder-violet" ) );
        QStringList content = col.contentTypes();
        if ( content.size() == 1 || (content.size() == 2 && content.contains( Collection::collectionMimeType() )) ) {
          if ( content.contains( QLatin1String( "text/x-vcard" ) ) || content.contains( QLatin1String( "text/directory" ) ) || content.contains( QLatin1String( "text/vcard" ) ) )
            return SmallIcon( QLatin1String( "kmgroupware_folder_contacts" ) );
          // TODO: add all other content types and/or fix their mimetypes
          if ( content.contains( QLatin1String( "akonadi/event" ) ) || content.contains( QLatin1String( "text/ical" ) ) )
            return SmallIcon( QLatin1String( "kmgroupware_folder_calendar" ) );
          if ( content.contains( QLatin1String( "akonadi/task" ) ) )
            return SmallIcon( QLatin1String( "kmgroupware_folder_tasks" ) );
          return SmallIcon( QLatin1String( "folder" ) );
        } else if ( content.isEmpty() ) {
          return SmallIcon( QLatin1String( "folder-grey" ) );
        } else
          return SmallIcon( QLatin1String( "folder-orange" ) ); // mixed stuff
      }
      break;
    case CollectionIdRole:
      return col.id();
    case CollectionContentTypesRole:
      return QVariant(col.contentTypes());
    case ChildCreatableRole:
      return canCreateCollection( index );
  }
  return QVariant();
}

QModelIndex CollectionModel::index( int row, int column, const QModelIndex & parent ) const
{
  if (column >= columnCount() || column < 0) return QModelIndex();

  QList<int> list;
  if ( !parent.isValid() )
    list = d->childCollections.value( Collection::root().id() );
  else
  {
    if (parent.column() > 0)
       return QModelIndex();
    list = d->childCollections.value( parent.internalId() );
  }

  if ( row < 0 || row >= list.size() )
    return QModelIndex();
  if ( !d->collections.contains( list.at(row) ) )
    return QModelIndex();
  return createIndex( row, column, d->collections.value( list.at(row) ).id() );
}

QModelIndex CollectionModel::parent( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return QModelIndex();

  Collection col = d->collections.value( index.internalId() );
  if ( !col.isValid() )
    return QModelIndex();

  Collection parentCol = d->collections.value( col.parent() );
  if ( !parentCol.isValid() )
    return QModelIndex();

  QList<int> list;
  list = d->childCollections.value( parentCol.parent() );

  int parentRow = list.indexOf( parentCol.id() );
  if ( parentRow < 0 )
    return QModelIndex();

  return createIndex( parentRow, 0, parentCol.id() );
}

int CollectionModel::rowCount( const QModelIndex & parent ) const
{
  QList<int> list;
  if ( parent.isValid() )
    list = d->childCollections.value( parent.internalId() );
  else
    list = d->childCollections.value( Collection::root().id() );

  return list.size();
}

QVariant CollectionModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
  if ( section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole )
    return i18nc( "@title:column, name of a thing", "Name" );
  return QAbstractItemModel::headerData( section, orientation, role );
}

bool CollectionModel::removeRowFromModel( int row, const QModelIndex & parent )
{
  QList<int> list;
  Collection parentCol;
  if ( parent.isValid() ) {
    parentCol = d->collections.value( parent.internalId() );
    Q_ASSERT( parentCol.id() == parent.internalId() );
    list = d->childCollections.value( parentCol.id() );
  } else {
    parentCol = Collection::root();
    list = d->childCollections.value( Collection::root().id() );
  }
  if ( row < 0 || row  >= list.size() ) {
    kWarning( 5250 ) << "Index out of bounds:" << row <<" parent:" << parentCol.id();
    return false;
  }

  beginRemoveRows( parent, row, row );
  int delColId = list.takeAt( row );
  foreach( int childColId, d->childCollections[ delColId ] )
    d->collections.remove( childColId );
  d->collections.remove( delColId );
  d->childCollections.remove( delColId ); // remove children of deleted collection
  d->childCollections.insert( parentCol.id(), list ); // update children of parent
  endRemoveRows();

  return true;
}

QModelIndex CollectionModel::indexForId( int id, int column )
{
  if ( !d->collections.contains( id ) )
    return QModelIndex();

  int parentId = d->collections.value( id ).parent();
  // check if parent still exist or if this is an orphan collection
  if ( parentId != Collection::root().id() && !d->collections.contains( parentId ) )
    return QModelIndex();

  QList<int> list = d->childCollections.value( parentId );
  int row = list.indexOf( id );

  if ( row >= 0 )
    return createIndex( row, column, d->collections.value( list.at(row) ).id() );
  return QModelIndex();
}

bool CollectionModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
  if ( index.column() == 0 && role == Qt::EditRole ) {
    // rename collection
    Collection col = d->collections.value( index.internalId() );
    if ( !col.isValid() || value.toString().isEmpty() )
      return false;
    CollectionModifyJob *job = new CollectionModifyJob( col, d->session );
    job->setName( value.toString() );
    connect( job, SIGNAL(result(KJob*)), SLOT(editDone(KJob*)) );
    return true;
  }
  return QAbstractItemModel::setData( index, value, role );
}

Qt::ItemFlags CollectionModel::flags( const QModelIndex & index ) const
{
  Qt::ItemFlags flags = QAbstractItemModel::flags( index );

  flags = flags | Qt::ItemIsDragEnabled;

  Collection col;
  if ( index.isValid() ) {
    col = d->collections.value( index.internalId() );
    Q_ASSERT( col.isValid() );
  }
  else
    return flags | Qt::ItemIsDropEnabled; // HACK Workaround for a probable bug in Qt

  if ( col.isValid() ) {
    if ( col.type() != Collection::VirtualParent )  {
      if ( index.column() == 0 )
        flags = flags | Qt::ItemIsEditable;
      if ( col.type() != Collection::Virtual )
        flags = flags | Qt::ItemIsDropEnabled;
    }
  }

  return flags;
}

bool CollectionModel::canCreateCollection( const QModelIndex & parent ) const
{
  if ( !parent.isValid() )
    return false; // FIXME: creation of top-level collections??

  Collection col = d->collections.value( parent.internalId() );
  if ( col.type() == Collection::Virtual || col.type() == Collection::VirtualParent )
    return false;
  if ( !col.contentTypes().contains( Collection::collectionMimeType() ) )
    return false;

  return true;
}

Qt::DropActions CollectionModel::supportedDropActions() const
{
  return Qt::CopyAction | Qt::MoveAction;
}

QStringList CollectionModel::mimeTypes() const
{
  return QStringList() << QLatin1String( "text/uri-list" );
}

bool CollectionModel::supportsContentType(const QModelIndex & index, const QStringList & contentTypes)
{
  if ( !index.isValid() )
    return false;
  Collection col = d->collections.value( index.internalId() );
  Q_ASSERT( col.isValid() );
  QStringList ct = col.contentTypes();
  foreach ( QString a, ct ) {
    if ( contentTypes.contains( a ) )
      return true;
  }
  return false;
}

QMimeData *CollectionModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *data = new QMimeData();
    KUrl::List urls;
    foreach ( QModelIndex index, indexes ) {
        if ( index.column() != 0 )
          continue;

        urls << Collection( index.internalId() ).url();
    }
    urls.populateMimeData( data );

    return data;
}

bool CollectionModel::dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent)
{
  if ( !(action & supportedDropActions()) )
    return false;

  // handle drops onto items as well as drops between items
  QModelIndex idx;
  if ( row >= 0 && column >= 0 )
    idx = index( row, column, parent );
  else
    idx = parent;

  if ( !idx.isValid() )
    return false;

  const Collection parentCol = d->collections.value( idx.internalId() );
  if (!parentCol.isValid())
    return false;

  // we try to drop data not coming with the akonadi:// url
  // find a type the target collection supports
  foreach ( QString type, data->formats() ) {
    if ( !supportsContentType( idx, QStringList( type ) ) )
      continue;

    QByteArray item = data->data( type );
    // HACK for some unknown reason the data is sometimes 0-terminated...
    if ( !item.isEmpty() && item.at( item.size() - 1 ) == 0 )
      item.resize( item.size() - 1 );

    Item it;
    it.setMimeType( type );
    it.addPart( Item::PartBody, item );

    ItemAppendJob *job = new ItemAppendJob( it, parentCol, d->session );
    connect( job, SIGNAL(result(KJob*)), SLOT(appendDone(KJob*)) );
    return job->exec();
  }

  if ( !KUrl::List::canDecode( data ) )
    return false;

  // data contains an url list
  bool success = false;
  KUrl::List urls = KUrl::List::fromMimeData( data );
  foreach( KUrl url, urls ) {
    if ( Collection::urlIsValid( url ) )
    {
      Collection col = Collection::fromUrl( url );
      if (action == Qt::MoveAction) {
        CollectionModifyJob *job = new CollectionModifyJob( col, d->session );
        job->setParent( parentCol );
        connect( job, SIGNAL(result(KJob*)), SLOT(appendDone(KJob*)) );
        job->exec();
        if ( !job->exec() ) {
          success = false;
          break;
        } else {
          success = true; // continue, there might be other urls
        }
      }
      else { // TODO A Copy Collection Job
        return false;
      }
    }
    else if ( Item::urlIsValid( url ) )
    {
      // TODO Extract mimetype from url and check if collection accepts it
      DataReference ref = Item::fromUrl( url );
      if (action == Qt::MoveAction) {
        ItemStoreJob *job = new ItemStoreJob( Item( ref ), d->session );
        job->setCollection( parentCol );
        connect( job, SIGNAL(result(KJob*)), SLOT(appendDone(KJob*)) );
        if ( !job->exec() ) {
          success = false;
          break;
        } else {
          success = true; // continue, there might be other urls
        }
      }
      else if ( action == Qt::CopyAction ) {
      // TODO Wait for a job allowing to copy on server side.
      }
      else {
        return false;
      }
    }
  }

  return success;
}

Collection CollectionModel::collectionForId(int id) const
{
  return d->collections.value( id );
}

void CollectionModel::fetchCollectionStatus(bool enable)
{
  d->fetchStatus = enable;
  d->monitor->fetchCollectionStatus( enable );
}



#include "collectionmodel.moc"
