/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>
   Copyright (C) 2012 Sérgio Martins <iamsergio@gmail.com>

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

#include "calendarbase.h"
#include "calendarbase_p.h"
#include "incidencechanger.h"

#include <akonadi/item.h>
#include <akonadi/collection.h>

#include <KSystemTimeZones>

using namespace Akonadi;
using namespace KCalCore;

CalendarBasePrivate::CalendarBasePrivate( CalendarBase *qq ) : QObject()
                                                             , mIncidenceChanger( new IncidenceChanger() )
                                                             , mBatchInsertionCancelled( false )
                                                             , q( qq )
{
  connect( mIncidenceChanger,
           SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
           SLOT(slotCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );

  connect( mIncidenceChanger,
           SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
           SLOT(slotDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)) );

  connect( mIncidenceChanger,
           SIGNAL(modifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
           SLOT(slotModifyFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );

  mIncidenceChanger->setDestinationPolicy( IncidenceChanger::DestinationPolicyAsk );
  mIncidenceChanger->setGroupwareCommunication( false );
  mIncidenceChanger->setHistoryEnabled( false );
}

CalendarBasePrivate::~CalendarBasePrivate()
{
  delete mIncidenceChanger;
  mIncidenceChanger = 0;
}

void CalendarBasePrivate::internalInsert( const Akonadi::Item &item )
{
  Q_ASSERT( item.isValid() );
  mItemById.insert( item.id(), item );
  KCalCore::Incidence::Ptr incidence= item.payload<KCalCore::Incidence::Ptr>();
  Q_ASSERT( incidence );
  if ( incidence ) {
    //kDebug() << "Inserting incidence in calendar. id=" << item.id() << "uid=" << incidence->uid();
    const QString uid = incidence->uid();
    Q_ASSERT( !uid.isEmpty() );

    if ( mItemIdByUid.contains( uid ) && mItemIdByUid[uid] != item.id() ) {
      // We only allow duplicate UIDs if they have the same item id, for example
      // when using virtual folders.
      kWarning() << "Discarding duplicate incidence with uid=" << uid
                 << "and summary " << incidence->summary();
      return;
    }

    mItemIdByUid.insert( incidence->uid(), item.id() );

    // Insert parent relationships
    const QString parentUid = incidence->relatedTo();
    if ( !parentUid.isEmpty() ) {
      mParentUidToChildrenUid[parentUid].append( incidence->uid() );
    }
    // Must be the last one due to re-entrancy
    q->MemoryCalendar::addIncidence( incidence );
  }
}

void CalendarBasePrivate::internalRemove( const Akonadi::Item &item )
{
  Q_ASSERT( item.isValid() );
  Q_ASSERT( item.hasPayload<KCalCore::Incidence::Ptr>() );
  Incidence::Ptr incidence = q->incidence( item.payload<KCalCore::Incidence::Ptr>()->uid() );


  // Null incidence means it was deleted via CalendarBase::deleteIncidence(), but then
  // the ETMCalendar received the monitor notification and tried to delete it again.
  if ( incidence ) {
    mItemById.remove( item.id() );
    // kDebug() << "Deleting incidence from calendar .id=" << item.id() << "uid=" << incidence->uid();
    mItemIdByUid.remove( incidence->uid() );

    mParentUidToChildrenUid.remove( incidence->uid() );
    const QString parentUid = incidence->relatedTo();
    if ( !parentUid.isEmpty() ) {
      mParentUidToChildrenUid[parentUid].removeAll( incidence->uid() );
    }
    // Must be the last one due to re-entrancy
    q->MemoryCalendar::deleteIncidence( incidence );
  }
}

void CalendarBasePrivate::deleteAllIncidencesOfType( const QString &mimeType )
{
  QHash<Item::Id, Item>::iterator i;
  Item::List incidences;
  for( i = mItemById.begin(); i != mItemById.end(); ++i ) {
    if ( i.value().payload<KCalCore::Incidence::Ptr>()->mimeType() == mimeType )
      incidences.append( i.value() );
  }

  mIncidenceChanger->deleteIncidences( incidences );
}

void CalendarBasePrivate::slotDeleteFinished( int changeId,
                                              const QVector<Akonadi::Item::Id> &itemIds,
                                              IncidenceChanger::ResultCode resultCode,
                                              const QString &errorMessage )
{
  Q_UNUSED( changeId );
  if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
    foreach( const Akonadi::Item::Id &id, itemIds ) {
      if ( mItemById.contains( id ) )
        internalRemove( mItemById.value( id ) );
    }
  }
  emit q->deleteFinished( resultCode == IncidenceChanger::ResultCodeSuccess, errorMessage );
}

void CalendarBasePrivate::slotCreateFinished( int changeId,
                                              const Akonadi::Item &item,
                                              IncidenceChanger::ResultCode resultCode,
                                              const QString &errorMessage )
{
  Q_UNUSED( changeId );
  Q_UNUSED( item );
  if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
    internalInsert( item );
  }
  emit q->createFinished( resultCode == IncidenceChanger::ResultCodeSuccess, errorMessage );
}

//TODO: unit-test this
void CalendarBasePrivate::slotModifyFinished( int changeId,
                                              const Akonadi::Item &item,
                                              IncidenceChanger::ResultCode resultCode,
                                              const QString &errorMessage )
{
  Q_UNUSED( changeId );
  Q_UNUSED( item );
  if ( resultCode == IncidenceChanger::ResultCodeSuccess ) {
    KCalCore::IncidenceBase::Ptr incidence = item.payload<KCalCore::IncidenceBase::Ptr>();
    Q_ASSERT( incidence );
    //update our local one
    *(static_cast<IncidenceBase*>(q->incidence( incidence->uid() ).data() ) ) = *(incidence.data());
  }
  emit q->modifyFinished( resultCode == IncidenceChanger::ResultCodeSuccess, errorMessage );
}

CalendarBase::CalendarBase() : MemoryCalendar( KSystemTimeZones::local() )
                             , d_ptr( new CalendarBasePrivate( this ) )
{
}

CalendarBase::CalendarBase( CalendarBasePrivate *const dd ) : MemoryCalendar( KSystemTimeZones::local() )
                                                            , d_ptr( dd )
{
}

CalendarBase::~CalendarBase()
{
}

Akonadi::Item CalendarBase::item( Akonadi::Item::Id id ) const
{
  Q_D(const CalendarBase);
  Akonadi::Item i;
  if ( d->mItemById.contains( id ) ) {
    i = d->mItemById[id];
  } else {
    kDebug() << "Can't find any item with id " << id;
  }
  return i;
}

Akonadi::Item CalendarBase::item( const QString &uid ) const
{
  Q_D(const CalendarBase);
  Q_ASSERT( !uid.isEmpty() );
  Akonadi::Item i;
  if ( d->mItemIdByUid.contains( uid ) ) {
    const Akonadi::Item::Id id = d->mItemIdByUid[uid];
    if ( !d->mItemById.contains( id ) ) {
      kError() << "Item with id " << id << "(uid=" << uid << ") not found, but in uid map";
      Q_ASSERT_X( false, "CalendarBase::item", "not in mItemById" );
    }
    i = d->mItemById[id];
  } else {
    kDebug() << "Can't find any incidence with uid " << uid;
  }
  return i;
}

Akonadi::Item::List CalendarBase::itemList( const KCalCore::Incidence::List &incidences ) const
{
  Akonadi::Item::List items;

  foreach( const KCalCore::Incidence::Ptr &incidence, incidences ) {
    if ( incidence ) {
      items << item( incidence->uid() );
    } else {
      items << Akonadi::Item();
    }
  }

  return items;
}

KCalCore::Incidence::List CalendarBase::childIncidences( const Akonadi::Item::Id &parentId ) const
{
  Q_D(const CalendarBase);
  KCalCore::Incidence::List childs;

  if ( d->mItemById.contains( parentId ) ) {
    const Akonadi::Item item = d->mItemById.value( parentId );
    Q_ASSERT( item.isValid() );
    Q_ASSERT( item.hasPayload<KCalCore::Incidence::Ptr>() );

    childs = childIncidences( item.payload<KCalCore::Incidence::Ptr>()->uid() );
  }

  return childs;
}

KCalCore::Incidence::List CalendarBase::childIncidences( const QString &parentUid ) const
{
  Q_D(const CalendarBase);
  KCalCore::Incidence::List children;
  const QStringList uids = d->mParentUidToChildrenUid.value( parentUid );
  Q_FOREACH( const QString &uid, uids ) {
    Incidence::Ptr child = incidence( uid );
    if ( child )
      children.append( child );
    else
      kWarning() << "Invalid child with uid " << uid;
  }
  return children;
}

Akonadi::Item::List CalendarBase::childItems( const Akonadi::Item::Id &parentId ) const
{
  Q_D(const CalendarBase);
  Akonadi::Item::List childs;

  if ( d->mItemById.contains( parentId ) ) {
    const Akonadi::Item item = d->mItemById.value( parentId );
    Q_ASSERT( item.isValid() );
    Q_ASSERT( item.hasPayload<KCalCore::Incidence::Ptr>() );

    childs = childItems( item.payload<KCalCore::Incidence::Ptr>()->uid() );
  }

  return childs;
}

Akonadi::Item::List CalendarBase::childItems( const QString &parentUid ) const
{
  Q_D(const CalendarBase);
  Akonadi::Item::List children;
  const QStringList uids = d->mParentUidToChildrenUid.value( parentUid );
  Q_FOREACH( const QString &uid, uids ) {
    Akonadi::Item child = item( uid );
    if ( child.isValid() && child.hasPayload<KCalCore::Incidence::Ptr>() )
      children.append( child );
    else
      kWarning() << "Invalid child with uid " << uid;
  }
  return children;
}

bool CalendarBase::addEvent( const KCalCore::Event::Ptr &event )
{
  return addIncidence( event );
}

bool CalendarBase::deleteEvent( const KCalCore::Event::Ptr &event )
{
  return deleteIncidence( event );
}

void CalendarBase::deleteAllEvents()
{
  Q_D(CalendarBase);
  d->deleteAllIncidencesOfType( Event::eventMimeType() );
}

bool CalendarBase::addTodo( const KCalCore::Todo::Ptr &todo )
{
  return addIncidence( todo );
}

bool CalendarBase::deleteTodo( const KCalCore::Todo::Ptr &todo )
{
  return deleteIncidence( todo );
}

void CalendarBase::deleteAllTodos()
{
  Q_D(CalendarBase);
  d->deleteAllIncidencesOfType( Todo::todoMimeType() );
}

bool CalendarBase::addJournal( const KCalCore::Journal::Ptr &journal )
{
  return addIncidence( journal );
}

bool CalendarBase::deleteJournal( const KCalCore::Journal::Ptr &journal )
{
  return deleteIncidence( journal );
}

void CalendarBase::deleteAllJournals()
{
  Q_D(CalendarBase);
  d->deleteAllIncidencesOfType( Journal::journalMimeType() );
}

bool CalendarBase::addIncidence( const KCalCore::Incidence::Ptr &incidence )
{
  //TODO: Parent for dialogs
  Q_D(CalendarBase);

  // User canceled on the collection selection dialog
  if ( batchAdding() && d->mBatchInsertionCancelled )
    return -1;

  Akonadi::Collection collection;

  if ( batchAdding() && d->mCollectionForBatchInsertion.isValid() )
    collection = d->mCollectionForBatchInsertion;

  const int changeId = d->mIncidenceChanger->createIncidence( incidence, collection );

  if ( batchAdding() ) {
    const Akonadi::Collection lastCollection = d->mIncidenceChanger->lastCollectionUsed();
    if ( changeId != -1 && !lastCollection.isValid() ) {
      d->mBatchInsertionCancelled = true;
    } else if ( lastCollection.isValid() && !d->mCollectionForBatchInsertion.isValid() ) {
      d->mCollectionForBatchInsertion = d->mIncidenceChanger->lastCollectionUsed();
    }
  }

  return changeId != -1;
}

bool CalendarBase::deleteIncidence( const KCalCore::Incidence::Ptr &incidence )
{
  Q_D(CalendarBase);
  Q_ASSERT( incidence );
  Akonadi::Item item_ = item( incidence->uid() );
  return -1 != d->mIncidenceChanger->deleteIncidence( item_ );
}

bool CalendarBase::modifyIncidence( const KCalCore::IncidenceBase::Ptr &newIncidence )
{
  Q_D(CalendarBase);
  Q_ASSERT( newIncidence );
  const Akonadi::Item item_ = item( newIncidence->uid() );
  return -1 != d->mIncidenceChanger->modifyIncidence( item_ );
}

void CalendarBase::setWeakPointer( const QWeakPointer<CalendarBase> &pointer )
{
  Q_D(CalendarBase);
  d->mWeakPointer = pointer;
}

QWeakPointer<CalendarBase> CalendarBase::weakPointer() const
{
  Q_D(const CalendarBase);
  return d->mWeakPointer;
}

IncidenceChanger* CalendarBase::incidenceChanger() const
{
  Q_D(const CalendarBase);
  return d->mIncidenceChanger;
}

void CalendarBase::startBatchAdding()
{
  KCalCore::MemoryCalendar::startBatchAdding();
}

void CalendarBase::endBatchAdding()
{
  Q_D(CalendarBase);
  d->mCollectionForBatchInsertion = Akonadi::Collection();
  d->mBatchInsertionCancelled = false;
  KCalCore::MemoryCalendar::endBatchAdding();
}

#include "calendarbase.moc"
#include "calendarbase_p.moc"
