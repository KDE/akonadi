/*
    Copyright (c) 2006 - 2008 Volker Krause <vkrause@kde.org>

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
#include "collectionmodel_p.h"

#include "collectionmodifyjob.h"
#include "monitor.h"
#include "pastehelper.h"
#include "session.h"

#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>

#include <QtCore/QMimeData>
#include <QtCore/QTimer>
#include <QtGui/QPixmap>

using namespace Akonadi;

CollectionModel::CollectionModel( QObject * parent ) :
    QAbstractItemModel( parent ),
    d_ptr( new CollectionModelPrivate( this ) )
{
  Q_D( CollectionModel );
  d->session = new Session( QByteArray("CollectionModel-") + QByteArray::number( qrand() ), this );
  QTimer::singleShot( 0, this, SLOT(init()) );

  // monitor collection changes
  d->monitor = new Monitor();
  d->monitor->monitorCollection( Collection::root() );
  d->monitor->fetchCollection( true );

  // ### Hack to get the kmail resource folder icons
  KIconLoader::global()->addAppDir( QLatin1String( "kmail" ) );
  KIconLoader::global()->addAppDir( QLatin1String( "kdepim" ) );
}

CollectionModel::~CollectionModel()
{
  Q_D( CollectionModel );
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
  const Q_D( CollectionModel );
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
        if ( col.type() == Collection::Structural )
          return SmallIcon( QLatin1String( "folder-grey" ) );
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
    case CollectionRole:
      return QVariant::fromValue( col );
    case CollectionContentTypesRole:
      return QVariant(col.contentTypes());
    case ChildCreatableRole:
      return canCreateCollection( index );
  }
  return QVariant();
}

QModelIndex CollectionModel::index( int row, int column, const QModelIndex & parent ) const
{
  const Q_D( CollectionModel );
  if (column >= columnCount() || column < 0) return QModelIndex();

  QList<Collection::Id> list;
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
  return createIndex( row, column, reinterpret_cast<void*>( d->collections.value( list.at(row) ).id() ) );
}

QModelIndex CollectionModel::parent( const QModelIndex & index ) const
{
  const Q_D( CollectionModel );
  if ( !index.isValid() )
    return QModelIndex();

  Collection col = d->collections.value( index.internalId() );
  if ( !col.isValid() )
    return QModelIndex();

  Collection parentCol = d->collections.value( col.parent() );
  if ( !parentCol.isValid() )
    return QModelIndex();

  QList<Collection::Id> list;
  list = d->childCollections.value( parentCol.parent() );

  int parentRow = list.indexOf( parentCol.id() );
  if ( parentRow < 0 )
    return QModelIndex();

  return createIndex( parentRow, 0, reinterpret_cast<void*>( parentCol.id() ) );
}

int CollectionModel::rowCount( const QModelIndex & parent ) const
{
  const  Q_D( CollectionModel );
  QList<Collection::Id> list;
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
  Q_D( CollectionModel );
  QList<Collection::Id> list;
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
  Collection::Id delColId = list.takeAt( row );
  foreach( Collection::Id childColId, d->childCollections[ delColId ] )
    d->collections.remove( childColId );
  d->collections.remove( delColId );
  d->childCollections.remove( delColId ); // remove children of deleted collection
  d->childCollections.insert( parentCol.id(), list ); // update children of parent
  endRemoveRows();

  return true;
}

QModelIndex CollectionModel::indexForId( Collection::Id id, int column )
{
  Q_D( CollectionModel );
  if ( !d->collections.contains( id ) )
    return QModelIndex();

  Collection::Id parentId = d->collections.value( id ).parent();
  // check if parent still exist or if this is an orphan collection
  if ( parentId != Collection::root().id() && !d->collections.contains( parentId ) )
    return QModelIndex();

  QList<Collection::Id> list = d->childCollections.value( parentId );
  int row = list.indexOf( id );

  if ( row >= 0 )
    return createIndex( row, column, reinterpret_cast<void*>( d->collections.value( list.at(row) ).id() ) );
  return QModelIndex();
}

bool CollectionModel::setData( const QModelIndex & index, const QVariant & value, int role )
{
  Q_D( CollectionModel );
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
  const Q_D( CollectionModel );
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
  const Q_D( CollectionModel );
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
  Q_D( CollectionModel );
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
  Q_D( CollectionModel );
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

  KJob *job = PasteHelper::paste( data, parentCol, action != Qt::MoveAction );
  connect( job, SIGNAL(result(KJob*)), SLOT(dropResult(KJob*)) );
  return true;
}

Collection CollectionModel::collectionForId(Collection::Id id) const
{
  const Q_D( CollectionModel );
  return d->collections.value( id );
}

void CollectionModel::fetchCollectionStatus(bool enable)
{
  Q_D( CollectionModel );
  d->fetchStatus = enable;
  d->monitor->fetchCollectionStatus( enable );
}

void CollectionModel::includeUnsubscribed(bool include)
{
  Q_D( CollectionModel );
  d->unsubscribed = include;
}



#include "collectionmodel.moc"
