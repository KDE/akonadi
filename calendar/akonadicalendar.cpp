/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>

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

#include "akonadicalendar.h"
#include "akonadicalendar_p.h"

#include <Akonadi/Item>
#include <Akonadi/Collection>

using namespace Akonadi;
using namespace KCalCore;

AkonadiCalendarPrivate::AkonadiCalendarPrivate( AkonadiCalendar *qq ) : QObject()
                                                                        , mIncidenceChanger( new IncidenceChanger() )
                                                                        , q( qq )
{
  connect( mIncidenceChanger,
           SIGNAL(createFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)),
           SLOT(slotCreateFinished(int,Akonadi::Item,Akonadi::IncidenceChanger::ResultCode,QString)) );

  connect( mIncidenceChanger,
           SIGNAL(deleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)),
           SLOT(slotDeleteFinished(int,QVector<Akonadi::Item::Id>,Akonadi::IncidenceChanger::ResultCode,QString)) );
}

AkonadiCalendarPrivate::~AkonadiCalendarPrivate()
{
  delete mIncidenceChanger;
}

void AkonadiCalendarPrivate::internalInsert( const Akonadi::Item &item )
{
  Q_ASSERT( item.isValid() );
  mItemById.insert( item.id(), item );
  KCalCore::Incidence::Ptr incidence= item.payload<KCalCore::Incidence::Ptr>();
  Q_ASSERT( incidence );
  if ( incidence ) {
    kDebug() << "Inserting incidence in calendar. id=" << item.id() << "uid=" << incidence->uid();
    Q_ASSERT( !incidence->uid().isEmpty() );
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

void AkonadiCalendarPrivate::internalRemove( const Akonadi::Item &item )
{
  Q_ASSERT( item.isValid() );
  Q_ASSERT( item.hasPayload<KCalCore::Incidence::Ptr>() );
  Incidence::Ptr incidence = q->incidence( item.payload<KCalCore::Incidence::Ptr>()->uid() );


  // Null incidence means it was deleted via AkonadiCalendar::deleteIncidence(), but then
  // the ETMCalendar received the monitor notification and tried to delete it again.
  if ( incidence ) {
    mItemById.remove( item.id() );
    kDebug() << "Deleting incidence from calendar .id=" << item.id() << "uid=" << incidence->uid();
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

void AkonadiCalendarPrivate::slotDeleteFinished( int changeId,
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

void AkonadiCalendarPrivate::slotCreateFinished( int changeId,
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

AkonadiCalendar::AkonadiCalendar( const KDateTime::Spec &timeSpec ) : MemoryCalendar( timeSpec )
                                                                      , d_ptr( new AkonadiCalendarPrivate( this ) )
{
}

AkonadiCalendar::AkonadiCalendar( AkonadiCalendarPrivate *const dd,
                                  const KDateTime::Spec &timeSpec ) : MemoryCalendar( timeSpec )
                                                                      , d_ptr( dd )
{
}

AkonadiCalendar::~AkonadiCalendar()
{
}

Akonadi::Item AkonadiCalendar::item( Akonadi::Item::Id id ) const
{
  Q_D(const AkonadiCalendar);
  Akonadi::Item i;
  if ( d->mItemById.contains( id ) ) {
    i = d->mItemById[id];
  } else {
    kDebug() << "Can't find any item with id " << id;
  }
  return i;
}

Akonadi::Item AkonadiCalendar::item( const QString &uid ) const
{
  Q_D(const AkonadiCalendar);
  Q_ASSERT( !uid.isEmpty() );
  Akonadi::Item i;
  if ( d->mItemIdByUid.contains( uid ) ) {
    const Akonadi::Item::Id id = d->mItemIdByUid[uid];
    if ( !d->mItemById.contains( id ) ) {
      kError() << "Item with id " << id << "(uid=" << uid << ") not found, but in uid map";
      Q_ASSERT_X( false, "AkonadiCalendar::item", "not in mItemById" );
    }
    i = d->mItemById[id];
  } else {
    kDebug() << "Can't find any incidence with uid " << uid;
  }
  return i;
}

KCalCore::Incidence::List AkonadiCalendar::childIncidences( const KCalCore::Incidence::Ptr &parent ) const
{
  Q_ASSERT( parent );
  Akonadi::Item item = this->item( parent->uid() );
  Q_ASSERT( item.isValid() );
  return childIncidences( item );
}

KCalCore::Incidence::List AkonadiCalendar::childIncidences( const Akonadi::Item &parent ) const
{
  Q_D(const AkonadiCalendar);
  KCalCore::Incidence::List children;
  const KCalCore::Incidence::Ptr incidence = parent.payload<KCalCore::Incidence::Ptr>();
  Q_ASSERT( incidence );
  const QStringList uids = d->mParentUidToChildrenUid.value( incidence->uid() );
  Q_FOREACH( const QString &uid, uids ) {
    Akonadi::Item item = this->item( uid );
    Q_ASSERT( item.isValid() );
    children.append( item.payload<KCalCore::Incidence::Ptr>() );
  }
  return children;
}

bool AkonadiCalendar::addEvent( const KCalCore::Event::Ptr &event )
{
  return addIncidence( event );
}

bool AkonadiCalendar::deleteEvent( const KCalCore::Event::Ptr &event )
{
  return deleteIncidence( event );
}

void AkonadiCalendar::deleteAllEvents()
{
  //TODO
}

bool AkonadiCalendar::addTodo( const KCalCore::Todo::Ptr &todo )
{
  return addIncidence( todo );
}

bool AkonadiCalendar::deleteTodo( const KCalCore::Todo::Ptr &todo )
{
  return deleteIncidence( todo );
}

void AkonadiCalendar::deleteAllTodos()
{
  //TODO
}

bool AkonadiCalendar::addJournal( const KCalCore::Journal::Ptr &journal )
{
  return addIncidence( journal );
}

bool AkonadiCalendar::deleteJournal( const KCalCore::Journal::Ptr &journal )
{
  return deleteIncidence( journal );
}

void AkonadiCalendar::deleteAllJournals()
{
  //TODO
}

bool AkonadiCalendar::addIncidence( const KCalCore::Incidence::Ptr &incidence )
{
  //TODO: Collection 1?
  //TODO: Parent for dialogs
  Q_D(AkonadiCalendar);
  return -1 != d->mIncidenceChanger->createIncidence( incidence, Akonadi::Collection() );
}

bool AkonadiCalendar::deleteIncidence( const KCalCore::Incidence::Ptr &incidence )
{
  Q_D(AkonadiCalendar);
  Q_ASSERT( incidence );
  Akonadi::Item item_ = item( incidence->uid() );
  return -1 != d->mIncidenceChanger->deleteIncidence( item_ );
}

void AkonadiCalendar::setWeakPointer( const QWeakPointer<AkonadiCalendar> &pointer )
{
  Q_D(AkonadiCalendar);
  d->mWeakPointer = pointer;
}

QWeakPointer<AkonadiCalendar> AkonadiCalendar::weakPointer() const
{
  Q_D(const AkonadiCalendar);
  return d->mWeakPointer;
}

#include "akonadicalendar.moc"
#include "akonadicalendar_p.moc"
