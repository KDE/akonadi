/*
   Copyright (C) 2011 Sérgio Martins <iamsergio@gmail.com>

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

#include "etmcalendar.h"
#include "etmcalendar_p.h"
#include "incidencefetchjob.h"
#include "calendarmodel_p.h"
#include "collectionselection_p.h"
#include "kcolumnfilterproxymodel_p.h"

#include <Akonadi/Item>
#include <Akonadi/Session>
#include <Akonadi/Collection>
#include <Akonadi/ChangeRecorder>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/EntityDisplayAttribute>
#include <Akonadi/EntityMimeTypeFilterModel>
#include <Akonadi/EntityTreeModel>
#include <KSelectionProxyModel>
#include <kcheckableproxymodel.h>

#include <QSortFilterProxyModel>
#include <QItemSelectionModel>

using namespace Akonadi;
using namespace KCalCore;

//TODO: implement batchAdding

ETMCalendarPrivate::ETMCalendarPrivate( ETMCalendar *qq ) : CalendarBasePrivate( qq )
                                                            , mETM( 0 )
                                                            , mFilteredETM( 0 )
                                                            , mCheckableProxyModel( 0 )
                                                            , mCollectionSelection( 0 )
                                                            , q( qq )
{
}

void ETMCalendarPrivate::init()
{
  Akonadi::Session *session = new Akonadi::Session( "ETMCalendar", q );
  Akonadi::ChangeRecorder *monitor = new Akonadi::ChangeRecorder( q );
  connect( monitor, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>) ),
           q, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>) ) );

  Akonadi::ItemFetchScope scope;
  scope.fetchFullPayload( true );
  scope.fetchAttribute<Akonadi::EntityDisplayAttribute>();

  monitor->setSession( session );
  monitor->setCollectionMonitored( Akonadi::Collection::root() );
  monitor->fetchCollection( true );
  monitor->setItemFetchScope( scope );
  monitor->setMimeTypeMonitored( "text/calendar", true );
  monitor->setMimeTypeMonitored( KCalCore::Event::eventMimeType(), true );
  monitor->setMimeTypeMonitored( KCalCore::Todo::todoMimeType(), true );
  monitor->setMimeTypeMonitored( KCalCore::Journal::journalMimeType(), true );
  mETM = new CalendarModel( monitor, q );
  mETM->setObjectName( "ETM" );

  setupFilteredETM();

  connect( mETM, SIGNAL(rowsInserted(QModelIndex,int,int)),
           SLOT(onRowsInserted(QModelIndex,int,int)) );
  connect( mETM, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
           SLOT(onDataChanged(QModelIndex,QModelIndex)) );
  connect( mETM, SIGNAL(rowsMoved(QModelIndex,int,int,QModelIndex,int)),
           SLOT(onRowsMoved(QModelIndex,int,int,QModelIndex,int)) );
  connect( mETM, SIGNAL(rowsRemoved(QModelIndex,int,int)),
           SLOT(onRowsRemoved(QModelIndex,int,int)) );

  connect( mFilteredETM, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
           SLOT(onDataChangedInFilteredModel(QModelIndex,QModelIndex)) );
  connect( mFilteredETM, SIGNAL(layoutChanged()),
           SLOT(onLayoutChangedInFilteredModel()) );
  connect( mFilteredETM, SIGNAL(modelReset()),
           SLOT(onModelResetInFilteredModel()) );
  connect( mFilteredETM, SIGNAL(rowsInserted(QModelIndex,int,int)),
           SLOT(onRowsInsertedInFilteredModel(QModelIndex,int,int)) );
  connect( mFilteredETM, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
           SLOT(onRowsAboutToBeRemovedInFilteredModel(QModelIndex,int,int)) );

  loadFromETM();
}

void ETMCalendarPrivate::setupFilteredETM()
{
  // Our calendar tree must be sorted.
  QSortFilterProxyModel *sortFilterProxy = new QSortFilterProxyModel( this );
  sortFilterProxy->setObjectName( "Sort" );
  sortFilterProxy->setDynamicSortFilter( true );
  sortFilterProxy->setSortCaseSensitivity( Qt::CaseInsensitive );
  sortFilterProxy->setSourceModel( mETM );

  // We're only interested in the CollectionTitle column
  KColumnFilterProxyModel *columnFilterProxy = new KColumnFilterProxyModel( this );
  columnFilterProxy->setSourceModel( sortFilterProxy );
  columnFilterProxy->setVisibleColumn( CalendarModel::CollectionTitle );
  columnFilterProxy->setObjectName( "Remove columns" );

  // Keep track of selected items.
  QItemSelectionModel* selectionModel = new QItemSelectionModel( columnFilterProxy );
  selectionModel->setObjectName( "Calendar Selection Model" );

  // Make item selection work by means of checkboxes.
  mCheckableProxyModel = new KCheckableProxyModel( this );
  mCheckableProxyModel->setSelectionModel( selectionModel );
  mCheckableProxyModel->setSourceModel( columnFilterProxy );
  mCheckableProxyModel->setObjectName( "Add checkboxes" );

  mCollectionSelection = new CollectionSelection( selectionModel, this );
  KSelectionProxyModel* selectionProxy = new KSelectionProxyModel( selectionModel );
  selectionProxy->setObjectName( "Only show items of selected collection" );
  selectionProxy->setFilterBehavior( KSelectionProxyModel::ChildrenOfExactSelection );
  selectionProxy->setSourceModel( mETM );

  mFilteredETM = new Akonadi::EntityMimeTypeFilterModel( this );
  mFilteredETM->setHeaderGroup( Akonadi::EntityTreeModel::ItemListHeaders );
  mFilteredETM->setSourceModel( selectionProxy );
  mFilteredETM->setSortRole( CalendarModel::SortRole );
  mFilteredETM->setObjectName( "Show headers" );
}

ETMCalendarPrivate::~ETMCalendarPrivate()
{
}

void ETMCalendarPrivate::loadFromETM()
{
  itemsAdded( itemsFromModel( mETM ) );
}

void ETMCalendarPrivate::clear()
{
  mCollectionMap.clear();
 
  itemsRemoved( mItemById.values() );
  Q_ASSERT( mItemById.isEmpty() );
  Q_ASSERT( mItemIdByUid.isEmpty() );

  //m_virtualItems.clear();
}

Akonadi::Item::List ETMCalendarPrivate::itemsFromModel( const QAbstractItemModel *model,
                                                          const QModelIndex &parentIndex,
                                                          int start, int end )
{
  const int endRow = end >= 0 ? end : model->rowCount( parentIndex ) - 1;
  Akonadi::Item::List items;
  int row = start;
  QModelIndex i = model->index( row, 0, parentIndex );
  while ( row <= endRow ) {
    const Akonadi::Item item = itemFromIndex( i );
    if ( item.hasPayload<KCalCore::Incidence::Ptr>() ) {
      items << item;
    } else {
      const QModelIndex childIndex = i.child( 0, 0 );
      if ( childIndex.isValid() ) {
        items << itemsFromModel( model, i );
      }
    }
    ++row;
    i = i.sibling( row, 0 );
  }
  return items;
}

Akonadi::Collection::List ETMCalendarPrivate::collectionsFromModel( const QAbstractItemModel *model,
                                                                    const QModelIndex &parentIndex,
                                                                    int start, int end )
{
  const int endRow = end >= 0 ? end : model->rowCount( parentIndex ) - 1;
  Akonadi::Collection::List collections;
  int row = start;
  QModelIndex i = model->index( row, 0, parentIndex );
  while ( row <= endRow ) {
    const Akonadi::Collection collection = collectionFromIndex( i );
    if ( collection.isValid() ) {
      collections << collection;
      QModelIndex childIndex = i.child( 0, 0 );
      if ( childIndex.isValid() ) {
        collections << collectionsFromModel( model, i );
      }
    }
    ++row;
    i = i.sibling( row, 0 );
  }
  return collections;
}

Akonadi::Item ETMCalendarPrivate::itemFromIndex( const QModelIndex &idx )
{
  Akonadi::Item item = idx.data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();
  item.setParentCollection(
    idx.data( Akonadi::EntityTreeModel::ParentCollectionRole ).value<Akonadi::Collection>() );
  return item;
}

void ETMCalendarPrivate::itemsAdded( const Akonadi::Item::List &items )
{
  foreach( const Akonadi::Item &item, items ) {
    internalInsert( item );
  }
}

void ETMCalendarPrivate::itemsRemoved( const Akonadi::Item::List &items )
{
  foreach( const Akonadi::Item &item, items ) {
    internalRemove( item );
  }
}

Akonadi::Collection ETMCalendarPrivate::collectionFromIndex( const QModelIndex &index )
{
  return index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
}

void ETMCalendarPrivate::onRowsInserted( const QModelIndex &index,
                                         int start, int end )
{
  Akonadi::Collection::List collections = collectionsFromModel( mETM, index,
                                                                start, end );

  foreach ( const Akonadi::Collection &collection, collections ) {
    mCollectionMap[collection.id()] = collection;
  }

  if ( !collections.isEmpty() )
    emit q->collectionsAdded( collections );
}

void ETMCalendarPrivate::onRowsRemoved( const QModelIndex &index, int start, int end )
{
  Akonadi::Collection::List collections = collectionsFromModel( mETM, index, start, end );
  foreach ( const Akonadi::Collection &collection, collections ) {
    mCollectionMap.remove( collection.id() );
  }

  if ( !collections.isEmpty() )
    emit q->collectionsRemoved( collections );
}

void ETMCalendarPrivate::onDataChanged( const QModelIndex &topLeft,
                                        const QModelIndex &bottomRight )
{
  // We only update collections, because items are handled in the filtered model
  Q_ASSERT( topLeft.row() <= bottomRight.row() );
  const int endRow = bottomRight.row();
  QModelIndex i( topLeft );
  int row = i.row();
  while ( row <= endRow ) {
    const Akonadi::Collection col = collectionFromIndex( i );
    if ( col.isValid() ) {
      // Attributes might have changed, store the new collection and discard the old one
      mCollectionMap.insert( col.id(), col );
    }
    ++row;
  }
}

void ETMCalendarPrivate::onRowsMoved( const QModelIndex &sourceParent,
                                      int sourceStart,
                                      int sourceEnd,
                                      const QModelIndex &destinationParent,
                                      int destinationRow )
{
  //TODO
  Q_UNUSED( sourceParent );
  Q_UNUSED( sourceStart );
  Q_UNUSED( sourceEnd );
  Q_UNUSED( destinationParent );
  Q_UNUSED( destinationRow );
}

void ETMCalendarPrivate::onLayoutChangedInFilteredModel()
{
  clear();
  loadFromETM();
}

void ETMCalendarPrivate::onModelResetInFilteredModel()
{
  clear();
  loadFromETM();
}

void ETMCalendarPrivate::onDataChangedInFilteredModel( const QModelIndex &topLeft,
                                                       const QModelIndex &bottomRight )
{
  Q_ASSERT( topLeft.row() <= bottomRight.row() );
  const int endRow = bottomRight.row();
  QModelIndex i( topLeft );
  int row = i.row();
  while ( row <= endRow ) {
    const Akonadi::Item item = itemFromIndex( i );
    if ( item.isValid() ) {
      Incidence::Ptr newIncidence = item.payload<KCalCore::Incidence::Ptr>();
      Q_ASSERT( newIncidence );
      Q_ASSERT( !newIncidence->uid().isEmpty() );
      IncidenceBase::Ptr existingIncidence = q->incidence( newIncidence->uid() );
      Q_ASSERT( existingIncidence );
      *(existingIncidence.data()) = *( newIncidence.data() );

      // The item needs updating too, revision changed.
      mItemById.insert( item.id(), item );
    }
    ++row;
    i = i.sibling( row, topLeft.column() );
  }
}

void ETMCalendarPrivate::onRowsInsertedInFilteredModel( const QModelIndex &index,
                                                        int start, int end )
{
  itemsAdded( itemsFromModel( mFilteredETM, index, start, end ) );
}

void ETMCalendarPrivate::onRowsAboutToBeRemovedInFilteredModel( const QModelIndex &index,
                                                                int start, int end )
{
  itemsRemoved( itemsFromModel( mFilteredETM, index, start, end ) );
}

ETMCalendar::ETMCalendar( const KDateTime::Spec &timeSpec )
            : CalendarBase( new ETMCalendarPrivate( this ), timeSpec )
{
  Q_D( ETMCalendar );
  d->init();
}

ETMCalendar::~ETMCalendar()
{
}

//TODO: move this up?
Akonadi::Collection ETMCalendar::collection( Akonadi::Collection::Id id ) const
{
  Q_D( const ETMCalendar );
  return d->mCollectionMap.value( id );
}

bool ETMCalendar::hasModifyRights( const QString &uid ) const
{
  return hasModifyRights( item( uid ) );
}

bool ETMCalendar::hasDeleteRights( const QString &uid ) const
{
  return hasDeleteRights( item( uid ) );
}

bool ETMCalendar::hasModifyRights( const Akonadi::Item &item ) const
{
  // if the users changes the rights, item.parentCollection()
  // can still have the old rights, so we use call collection()
  // which returns the updated one
  const Akonadi::Collection col = collection( item.storageCollectionId() );
  return col.rights() & Akonadi::Collection::CanChangeItem;
}

bool ETMCalendar::hasDeleteRights( const Akonadi::Item &item ) const
{
  // if the users changes the rights, item.parentCollection()
  // can still have the old rights, so we use call collection()
  // which returns the updated one
  const Akonadi::Collection col = collection( item.storageCollectionId() );
  return col.rights() & Akonadi::Collection::CanDeleteItem;
}

QAbstractItemModel *ETMCalendar::filteredModel() const
{
  Q_D( const ETMCalendar );
  return d->mFilteredETM;
}


QAbstractItemModel *ETMCalendar::unfilteredModel() const
{
  Q_D( const ETMCalendar );
  return d->mETM;
}

KCheckableProxyModel *ETMCalendar::checkableProxyModel() const
{
  Q_D( const ETMCalendar );
  return d->mCheckableProxyModel;
}

Akonadi::CollectionSelection* ETMCalendar::collectionSelection() const
{
  Q_D( const ETMCalendar );
  return d->mCollectionSelection;
}

#include "etmcalendar.moc"
#include "etmcalendar_p.moc"
