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

#include "collection.h"
#include "collectionstatusjob.h"
#include "collectionlistjob.h"
#include "collectionmodel.h"
#include "collectionmodifyjob.h"
#include "itemappendjob.h"
#include "monitor.h"
#include "session.h"

#include <kdebug.h>
#include <kiconloader.h>
#include <kcomponentdata.h>
#include <klocale.h>

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QMimeData>
#include <QtCore/QQueue>
#include <QtGui/QPixmap>

using namespace Akonadi;

class CollectionModel::Private
{
  public:
    QHash<int, Collection> collections;
    QHash<int, QList<int> > childCollections;
    Monitor* monitor;
    Session *session;
    QStringList mimeTypes;

    void updateSupportedMimeTypes( Collection col )
    {
      QList<QByteArray> l = col.contentTypes();
      for ( QList<QByteArray>::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it ) {
        if ( (*it) == Collection::collectionMimeType() )
          continue;
        if ( !mimeTypes.contains( QString::fromLatin1( *it ) ) )
          mimeTypes << QString::fromLatin1( *it );
      }
    }
};

CollectionModel::CollectionModel( QObject * parent ) :
    QAbstractItemModel( parent ),
    d( new Private() )
{
  d->session = new Session( QByteArray("CollectionModel-") + QByteArray::number( qrand() ), this );

  // start a list job
  CollectionListJob *job = new CollectionListJob( Collection::root(), CollectionListJob::Recursive, d->session );
  connect( job, SIGNAL(result(KJob*)), SLOT(listDone(KJob*)) );

  // monitor collection changes
  d->monitor = new Monitor();
  d->monitor->monitorCollection( Collection::root() );
  connect( d->monitor, SIGNAL(collectionChanged(int,QString)), SLOT(collectionChanged(int)) );
  connect( d->monitor, SIGNAL(collectionAdded(int,QString)), SLOT(collectionChanged(int)) );
  connect( d->monitor, SIGNAL(collectionRemoved(int,QString)), SLOT(collectionRemoved(int)) );

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
  Q_UNUSED( parent );
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
          return SmallIcon( QLatin1String( "server" ) );
        if ( col.type() == Collection::VirtualParent )
          return SmallIcon( QLatin1String( "edit-find" ) );
        if ( col.type() == Collection::Virtual )
          return SmallIcon( QLatin1String( "folder-green" ) );
        QList<QByteArray> content = col.contentTypes();
        if ( content.size() == 1 || (content.size() == 2 && content.contains( Collection::collectionMimeType() )) ) {
          if ( content.contains( "text/x-vcard" ) || content.contains( "text/vcard" ) )
            return SmallIcon( QLatin1String( "kmgroupware_folder_contacts" ) );
          // TODO: add all other content types and/or fix their mimetypes
          if ( content.contains( "akonadi/event" ) || content.contains( "text/ical" ) )
            return SmallIcon( QLatin1String( "kmgroupware_folder_calendar" ) );
          if ( content.contains( "akonadi/task" ) )
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
    case ChildCreatableRole:
      return canCreateCollection( index );
  }
  return QVariant();
}

QModelIndex CollectionModel::index( int row, int column, const QModelIndex & parent ) const
{
  QList<int> list;
  if ( !parent.isValid() )
    list = d->childCollections.value( Collection::root().id() );
  else
    list = d->childCollections.value( parent.internalId() );

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
    return i18n( "Name" );
  return QAbstractItemModel::headerData( section, orientation, role );
}

bool CollectionModel::removeRowFromModel( int row, const QModelIndex & parent )
{
  QList<int> list;
  Collection parentCol;
  if ( parent.isValid() ) {
    parentCol = d->collections.value( parent.internalId() );
    list = d->childCollections.value( parentCol.id() );
  } else
    list = d->childCollections.value( Collection::root().id() );
  if ( row < 0 || row  >= list.size() ) {
    kWarning() << k_funcinfo << "Index out of bounds: " << row << " parent: " << parentCol.id() << endl;
    return false;
  }

  beginRemoveRows( parent, row, row );
  int delColId = list.takeAt( row );
  d->childCollections.remove( delColId ); // remove children of deleted collection
  d->childCollections.insert( parentCol.id(), list ); // update children of parent
  endRemoveRows();

  return true;
}

void CollectionModel::collectionChanged( int collection )
{
  CollectionListJob *job = new CollectionListJob( Collection( collection ), CollectionListJob::Local, d->session );
  connect( job, SIGNAL(result(KJob*)), SLOT(listDone(KJob*)) );
  if ( d->collections.contains( collection ) ) {
    CollectionStatusJob *job = new CollectionStatusJob( d->collections.value( collection ), d->session );
    connect( job, SIGNAL(result(KJob*)), SLOT(updateDone(KJob*)) );
  }
}

void CollectionModel::collectionRemoved( int collection )
{
  QModelIndex colIndex = indexForId( collection );
  if ( colIndex.isValid() ) {
    QModelIndex parentIndex = parent( colIndex );
    // collection is still somewhere in the hierarchy
    removeRowFromModel( colIndex.row(), parentIndex );
  } else {
    if ( d->collections.contains( collection ) ) {
      // collection is orphan, ie. the parent has been removed already
      d->collections.remove( collection );
      d->childCollections.remove( collection );
    }
  }
}

void CollectionModel::updateDone( KJob * job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning() << k_funcinfo << "Job error: " << job->errorString() << endl;
  } else {
    CollectionStatusJob *csjob = static_cast<CollectionStatusJob*>( job );
    Collection result = csjob->collection();
    if ( !d->collections.contains( result.id() ) )
      kWarning() << k_funcinfo << "Got status response for non-existing collection: " << result.id() << endl;
    else {
      Collection col = d->collections.value( result.id() );
      foreach ( CollectionAttribute* attr, result.attributes() )
        col.addAttribute( attr->clone() );
      d->collections[ col.id() ] = col;
      d->updateSupportedMimeTypes( col );

      QModelIndex startIndex = indexForId( col.id() );
      QModelIndex endIndex = indexForId( col.id(), columnCount( parent( startIndex ) ) - 1 );
      emit dataChanged( startIndex, endIndex );
    }
  }
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

void CollectionModel::listDone( KJob * job )
{
  if ( job->error() ) {
    qWarning() << "Job error: " << job->errorString() << endl;
  } else {
    Collection::List collections = static_cast<CollectionListJob*>( job )->collections();

    // update model
    foreach( const Collection col, collections ) {
      if ( d->collections.contains( col.id() ) ) {
        // collection already known
        d->collections[ col.id() ] = col;
        QModelIndex startIndex = indexForId( col.id() );
        QModelIndex endIndex = indexForId( col.id(), columnCount( parent( startIndex ) ) - 1 );
        emit dataChanged( startIndex, endIndex );
        continue;
      }
      d->collections.insert( col.id(), col );
      QModelIndex parentIndex = indexForId( col.parent() );
      if ( parentIndex.isValid() || col.parent() == Collection::root().id() ) {
        beginInsertRows( parentIndex, rowCount( parentIndex ), rowCount( parentIndex ) );
        d->childCollections[ col.parent() ].append( col.id() );
        endInsertRows();
      } else {
        d->childCollections[ col.parent() ].append( col.id() );
      }

      d->updateSupportedMimeTypes( col );

      // start a status job for every collection to get message counts, etc.
      if ( col.type() != Collection::VirtualParent ) {
        CollectionStatusJob* csjob = new CollectionStatusJob( col, d->session );
        connect( csjob, SIGNAL(result(KJob*)), SLOT(updateDone(KJob*)) );
      }
    }

  }
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

  Collection col;
  if ( index.isValid() ) {
    col = d->collections.value( index.internalId() );
    Q_ASSERT( col.isValid() );
  }

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

void CollectionModel::editDone( KJob * job )
{
  if ( job->error() ) {
    qWarning() << "Edit failed: " << job->errorString();
  }
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
  return Qt::CopyAction; // | Qt::MoveAction;
}

QStringList CollectionModel::mimeTypes() const
{
  return d->mimeTypes;
}

bool CollectionModel::supportsContentType(const QModelIndex & index, const QStringList & contentTypes)
{
  if ( !index.isValid() )
    return false;
  Collection col = d->collections.value( index.internalId() );
  Q_ASSERT( col.isValid() );
  QList<QByteArray> ct = col.contentTypes();
  foreach ( QByteArray a, ct ) {
    if ( contentTypes.contains( QString::fromLatin1( a ) ) )
      return true;
  }
  return false;
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

  // find a type the target collection supports
  foreach ( QString type, data->formats() ) {
    if ( !supportsContentType( idx, QStringList( type ) ) )
      continue;
    const Collection col = d->collections.value( idx.internalId() );
    if ( !col.isValid() )
      return false;
    QByteArray item = data->data( type );
    // HACK for some unknown reason the data is sometimes 0-terminated...
    if ( !item.isEmpty() && item.at( item.size() - 1 ) == 0 )
      item.resize( item.size() - 1 );
    ItemAppendJob *job = new ItemAppendJob( col, type.toLatin1(), d->session );
    job->setData( item );
    connect( job, SIGNAL(result(KJob*)), SLOT(appendDone(KJob*)) );
    return true;
  }

  return false;
}

void CollectionModel::appendDone(KJob * job)
{
  if ( job->error() ) {
    kWarning() << "Append failed: " << job->errorString() << endl;
    // TODO: error handling
  }
}

Collection CollectionModel::collectionForId(int id) const
{
  return d->collections.value( id );
}

#include "collectionmodel.moc"
