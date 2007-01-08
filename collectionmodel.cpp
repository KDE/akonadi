/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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
#include "collectioncreatejob.h"
#include "collectionstatusjob.h"
#include "collectionlistjob.h"
#include "collectionmodel.h"
#include "collectionrenamejob.h"
#include "itemappendjob.h"
#include "jobqueue.h"
#include "monitor.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kinstance.h>
#include <klocale.h>
#include <kapplication.h>

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtCore/QMimeData>
#include <QtCore/QQueue>
#include <QtGui/QPixmap>

using namespace Akonadi;

class CollectionModel::Private
{
  public:
    enum EditType { None, Rename, Create, Delete };
    QHash<QString, Collection*> collections;
    QHash<QString, QList<QString> > childCollections;
    EditType currentEdit;
    Collection *editedCollection;
    QString editOldName;
    Monitor* monitor;
    JobQueue *queue;
    QStringList mimeTypes;

    void updateSupportedMimeTypes( Collection *col )
    {
      QList<QByteArray> l = col->contentTypes();
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
  d->currentEdit = Private::None;
  d->editedCollection = 0;

  d->queue = new JobQueue( this );

  // start a list job
  CollectionListJob *job = new CollectionListJob( Collection::prefix(), true, d->queue );
  connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(listDone(Akonadi::Job*)) );

  // monitor collection changes
  d->monitor = new Monitor();
  d->monitor->monitorCollection( Collection::root(), true );
  connect( d->monitor, SIGNAL(collectionChanged(QString)), SLOT(collectionChanged(QString)) );
  connect( d->monitor, SIGNAL(collectionAdded(QString)), SLOT(collectionChanged(QString)) );
  connect( d->monitor, SIGNAL(collectionRemoved(QString)), SLOT(collectionRemoved(QString)) );

  // ### Hack to get the kmail resource folder icons
  KIconLoader::global()->addAppDir( QLatin1String( "kmail" ) );
}

CollectionModel::~CollectionModel()
{
  d->childCollections.clear();
  qDeleteAll( d->collections );
  d->collections.clear();

  delete d->monitor;
  d->monitor = 0;

  delete d;
  d = 0;
}

int CollectionModel::columnCount( const QModelIndex & parent ) const
{
  Q_UNUSED( parent );
  return 1;
}

QVariant CollectionModel::data( const QModelIndex & index, int role ) const
{
  Collection *col = static_cast<Collection*>( index.internalPointer() );
  if ( index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole) ) {
    return col->name();
  }
  if ( index.column() == 0 && role == Qt::DecorationRole ) {
    if ( col->type() == Collection::Resource )
      return SmallIcon( QLatin1String( "server" ) );
    if ( col->type() == Collection::VirtualParent )
      return SmallIcon( QLatin1String( "find" ) );
    if ( col->type() == Collection::Virtual )
      return SmallIcon( QLatin1String( "folder_green" ) );
    QList<QByteArray> content = col->contentTypes();
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
      return SmallIcon( QLatin1String( "folder_grey" ) );
    } else
      return SmallIcon( QLatin1String( "folder_orange" ) ); // mixed stuff
  }
  return QVariant();
}

QModelIndex CollectionModel::index( int row, int column, const QModelIndex & parent ) const
{
  QList<QString> list;
  if ( !parent.isValid() )
    list = d->childCollections.value( Collection::root() );
  else
    list = d->childCollections.value( static_cast<Collection*>( parent.internalPointer() )->path() );

  if ( row < 0 || row >= list.size() )
    return QModelIndex();
  if ( !d->collections.contains( list[row] ) )
    return QModelIndex();
  return createIndex( row, column, d->collections.value( list[row] ) );
}

QModelIndex CollectionModel::parent( const QModelIndex & index ) const
{
  if ( !index.isValid() )
    return QModelIndex();

  Collection *col = static_cast<Collection*>( index.internalPointer() );
  QString parentPath = col->parent();
  Collection *parentCol = d->collections.value( parentPath );
  if ( !parentCol )
    return QModelIndex();

  QList<QString> list;
  list = d->childCollections.value( parentCol->parent() );

  int parentRow = list.indexOf( parentPath );
  if ( parentRow < 0 )
    return QModelIndex();

  return createIndex( parentRow, 0, parentCol );
}

int CollectionModel::rowCount( const QModelIndex & parent ) const
{
  QList<QString> list;
  if ( parent.isValid() )
    list = d->childCollections.value( static_cast<Collection*>( parent.internalPointer() )->path() );
  else
    list = d->childCollections.value( Collection::root() );

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
  QList<QString> list;
  QString parentPath;
  if ( parent.isValid() ) {
    parentPath = static_cast<Collection*>( parent.internalPointer() )->path();
    list = d->childCollections.value( parentPath );
  } else
    list = d->childCollections.value( Collection::root() );
  if ( row < 0 || row  >= list.size() ) {
    kWarning() << k_funcinfo << "Index out of bounds: " << row << " parent: " << parentPath << endl;
    return false;
  }

  beginRemoveRows( parent, row, row );
  QString path = list.takeAt( row );
  delete d->collections.take( path );
  d->childCollections.remove( path );
  d->childCollections.insert( parentPath, list );
  endRemoveRows();

  return true;
}

void CollectionModel::collectionChanged( const QString &path )
{
  if ( d->collections.contains( path ) ) {
    // update
    CollectionStatusJob *job = new CollectionStatusJob( path, d->queue );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(updateDone(Akonadi::Job*)) );
  } else {
    // new collection
    int index = path.lastIndexOf( Collection::delimiter() );
    QString parent;
    if ( index > 0 )
      parent = path.left( index );
    else
      parent = Collection::root();

    // re-list parent non-recursively
    CollectionListJob *job = new CollectionListJob( parent, false, d->queue );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(listDone(Akonadi::Job*)) );
    // list the new collection recursively
    job = new CollectionListJob( path, true, d->queue );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(listDone(Akonadi::Job*)) );
  }
}

void CollectionModel::collectionRemoved( const QString &path )
{
  QModelIndex colIndex = indexForPath( path );
  if ( colIndex.isValid() ) {
    QModelIndex parentIndex = parent( colIndex );
    // collection is still somewhere in the hierarchy
    removeRowFromModel( colIndex.row(), parentIndex );
  } else {
    if ( d->collections.contains( path ) ) {
      // collection is orphan, ie. the parent has been removed already
      delete d->collections.take( path );
      d->childCollections.remove( path );
    }
  }
}

void CollectionModel::updateDone( Job * job )
{
  if ( job->error() ) {
    // TODO: handle job errors
    kWarning() << k_funcinfo << "Job error: " << job->errorText() << endl;
  } else {
    CollectionStatusJob *csjob = static_cast<CollectionStatusJob*>( job );
    QString path = csjob->path();
    if ( !d->collections.contains( path ) )
      kWarning() << k_funcinfo << "Got status response for non-existing collection: " << path << endl;
    else {
      Collection *col = d->collections.value( path );
      foreach ( CollectionAttribute* attr, csjob->attributes() )
        col->addAttribute( attr );

      d->updateSupportedMimeTypes( col );

      QModelIndex startIndex = indexForPath( path );
      QModelIndex endIndex = indexForPath( path, columnCount( parent( startIndex ) ) - 1 );
      emit dataChanged( startIndex, endIndex );
    }
  }

  job->deleteLater();
}

QModelIndex CollectionModel::indexForPath( const QString &path, int column )
{
  if ( !d->collections.contains( path ) )
    return QModelIndex();

  QString parentPath = d->collections.value( path )->parent();
  // check if parent still exist or if this is an orphan collection
  if ( parentPath != Collection::root() && !d->collections.contains( parentPath ) )
    return QModelIndex();

  QList<QString> list = d->childCollections.value( parentPath );
  int row = list.indexOf( path );

  if ( row >= 0 )
    return createIndex( row, column, d->collections.value( list[row] ) );
  return QModelIndex();
}

void CollectionModel::listDone( Job * job )
{
  if ( job->error() ) {
    qWarning() << "Job error: " << job->errorText() << endl;
  } else {
    Collection::List collections = static_cast<CollectionListJob*>( job )->collections();

    // update model
    foreach( Collection* col, collections ) {
      if ( d->collections.contains( col->path() ) ) {
        // collection already known
        continue;
      }
      d->collections.insert( col->path(), col );
      QModelIndex parentIndex = indexForPath( col->parent() );
      if ( parentIndex.isValid() || col->parent() == Collection::root() ) {
        beginInsertRows( parentIndex, rowCount( parentIndex ), rowCount( parentIndex ) );
        d->childCollections[ col->parent() ].append( col->path() );
        endInsertRows();
      } else {
        d->childCollections[ col->parent() ].append( col->path() );
      }

      d->updateSupportedMimeTypes( col );

      // start a status job for every collection to get message counts, etc.
      if ( col->type() != Collection::VirtualParent ) {
        CollectionStatusJob* csjob = new CollectionStatusJob( col->path(), d->queue );
        connect( csjob, SIGNAL(done(Akonadi::Job*)), SLOT(updateDone(Akonadi::Job*)) );
      }
    }

  }
  job->deleteLater();
}

bool CollectionModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
  if ( d->currentEdit != Private::None )
    return false;
  if ( index.column() == 0 && role == Qt::EditRole ) {
    // rename collection
    d->editedCollection = static_cast<Collection*>( index.internalPointer() );
    d->currentEdit = Private::Rename;
    d->editOldName = d->editedCollection->name();
    d->editedCollection->setName( value.toString() );
    QString newPath;
    if ( d->editedCollection->parent() == Collection::root() )
      newPath = d->editedCollection->name();
    else
      newPath = d->editedCollection->parent() + Collection::delimiter() + d->editedCollection->name();
    CollectionRenameJob *job = new CollectionRenameJob( d->editedCollection->path(), newPath, d->queue );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(editDone(Akonadi::Job*)) );
    emit dataChanged( index, index );
    return true;
  }
  return QAbstractItemModel::setData( index, value, role );
}

Qt::ItemFlags CollectionModel::flags( const QModelIndex & index ) const
{
  Qt::ItemFlags flags = QAbstractItemModel::flags( index );

  Collection *col = 0;
  if ( index.isValid() ) {
    col = static_cast<Collection*>( index.internalPointer() );
    Q_ASSERT( col );
  }

  if ( col ) {
    if ( col->type() != Collection::VirtualParent )  {
      if ( d->currentEdit == Private::None && index.column() == 0 )
        flags = flags | Qt::ItemIsEditable;
      if ( col->type() != Collection::Virtual )
        flags = flags | Qt::ItemIsDropEnabled;
    }
  }

  return flags;
}

void CollectionModel::editDone( Job * job )
{
  if ( job->error() ) {
    qWarning() << "Edit failed: " << job->errorText() << " - reverting current transaction";
    // revert current transaction
    switch ( d->currentEdit ) {
      case Private::Create:
      {
        QModelIndex index = indexForPath( d->editedCollection->path() );
        QModelIndex parent = indexForPath( d->editedCollection->parent() );
        removeRowFromModel( index.row(), parent );
        d->editedCollection = 0;
        break;
      }
      case Private::Rename:
        d->editedCollection->setName( d->editOldName );
        QModelIndex index = indexForPath( d->editedCollection->path() );
        emit dataChanged( index, index );
    }
    // TODO: revert current change
  } else {
    // transaction done
  }
  d->currentEdit = Private::None;
  job->deleteLater();
}

bool CollectionModel::createCollection( const QModelIndex & parent, const QString & name )
{
  if ( !canCreateCollection( parent ) )
    return false;
  Collection *parentCol = static_cast<Collection*>( parent.internalPointer() );

  // create temporary fake collection, will be removed on error
  d->editedCollection = new Collection( parentCol->path() + Collection::delimiter() + name );
  d->editedCollection->setParent( parentCol->path() );
  if ( d->collections.contains( d->editedCollection->path() ) ) {
    delete d->editedCollection;
    d->editedCollection = 0;
    return false;
  }
  d->collections.insert( d->editedCollection->path(), d->editedCollection );
  beginInsertRows( parent, rowCount( parent ), rowCount( parent ) );
  d->childCollections[ d->editedCollection->parent() ].append( d->editedCollection->path() );
  endInsertRows();

  // start creation job
  CollectionCreateJob *job = new CollectionCreateJob( d->editedCollection->path(), d->queue );
  connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(editDone(Akonadi::Job*)) );

  d->currentEdit = Private::Create;
  return true;
}

bool CollectionModel::canCreateCollection( const QModelIndex & parent ) const
{
  if ( d->currentEdit != Private::None )
    return false;
  if ( !parent.isValid() )
    return false; // FIXME: creation of top-level collections??

  Collection *col = static_cast<Collection*>( parent.internalPointer() );
  if ( col->type() == Collection::Virtual || col->type() == Collection::VirtualParent )
    return false;
  if ( !col->contentTypes().contains( Collection::collectionMimeType() ) )
    return false;

  return true;
}

QString CollectionModel::pathForIndex( const QModelIndex & index ) const
{
  if ( index.isValid() )
    return static_cast<Collection*>( index.internalPointer() )->path();
  return QString();
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
  Collection *col = static_cast<Collection*>( index.internalPointer() );
  Q_ASSERT( col );
  QList<QByteArray> ct = col->contentTypes();
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
    QString path = pathForIndex( idx );
    QByteArray item = data->data( type );
    // HACK for some unknown reason the data is sometimes 0-terminated...
    if ( !item.isEmpty() && item.at( item.size() - 1 ) == 0 )
      item.resize( item.size() - 1 );
    ItemAppendJob *job = new ItemAppendJob( path, item, type.toLatin1(), d->queue );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(appendDone(Akonadi::Job*)) );
    return true;
  }

  return false;
}

void CollectionModel::appendDone(Job * job)
{
  if ( job->error() ) {
    kWarning() << "Append failed: " << job->errorText() << endl;
    // TODO: error handling
  }
  job->deleteLater();
}

#include "collectionmodel.moc"
