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

#ifndef _AKONADI_CALENDARBASE_H_
#define _AKONADI_CALENDARBASE_H_

#include "akonadi-calendar_export.h"

#include <akonadi/item.h>
#include <akonadi/collection.h>
#include <kcalcore/memorycalendar.h>
#include <kcalcore/incidence.h>
#include <KDE/KDateTime>

namespace Akonadi {

class CalendarBasePrivate;
class IncidenceChanger;

/**
* @short The base class for all akonadi aware calendars.
*
* Because it inherits KCalCore::Calendar, it provides seamless integration
* with KCalCore and KCalUtils libraries eliminating any need for adapter
* ( akonadi<->KCalCore ) classes.
*
* @see ETMCalendar
* @see FetchJobCalendar
*
* @author Sérgio Martins <sergio.martins@kdab.com>
* @since 4.11
*/
class AKONADI_CALENDAR_EXPORT CalendarBase : public KCalCore::MemoryCalendar
{
  Q_OBJECT
public:
  typedef QSharedPointer<CalendarBase> Ptr;

  /**
    * Constructs a CalendarBase object.
    */
  explicit CalendarBase( QObject *parent = 0 );

  /**
    * Destroys the calendar.
    */
  ~CalendarBase();

  /**
    * Returns the Item containing the incidence with uid @p uid or an invalid Item
    * if the incidence isn't found.
    * @see Use item(Incidence::Ptr) instead where possible. This function doesn't take exceptions (recurrenceId) into account (and thus always returns the main event).
    */
  Akonadi::Item item( const QString &uid ) const;

  /**
    * Returns the Item containing the incidence with uid @p uid or an invalid Item
    * if the incidence isn't found.
    */
  Akonadi::Item item( const KCalCore::Incidence::Ptr &incidence ) const;

  /**
    * Returns the Item with @p id or an invalid Item if not found.
    */
  Akonadi::Item item( Akonadi::Item::Id ) const;

  /**
   *  Returns the list of items contained in this calendar.
   *  @see incidences()
   */
  Akonadi::Item::List items() const;

  /**
   * Returns the list of items contained in this calendar that belong to the specified collection.
   * @see incidences()
   * // TODO_KDE5: merge with items() overload
   * @since 4.12
   */
  Akonadi::Item::List items( Akonadi::Collection::Id ) const;

  /**
    * Returns the item list that corresponds to the @p incidenceList.
    */
  Akonadi::Item::List itemList( const KCalCore::Incidence::List &incidenceList ) const;

  /**
    * Returns the child incidences of the parent identified by @p parentUid.
    * Only the direct childs are returned
    * @param parentUid identifier of the parent incidence
    */ //TODO: unit-test
  KCalCore::Incidence::List childIncidences( const QString &parentUid ) const;

  /**
    * Returns the child incidences of the parent identified by @p parentId.
    * Only the direct childs are returned
    * @param parentId identifier of the parent item
    */
  KCalCore::Incidence::List childIncidences( const Akonadi::Item::Id &parentId ) const;

  /**
    * Returns the child items of the parent identified by @p parentUid.
    * Only the direct childs are returned
    * @param parentUid identifier of the parent incidence
    */
  Akonadi::Item::List childItems( const QString &parentUid ) const;

  /**
    * Returns the child items of the parent identified by @p parentId.
    * Only the direct childs are returned
    * @param parentId identifier of the parent item
    */
  Akonadi::Item::List childItems( const Akonadi::Item::Id &parentId ) const;

  /**
    * Sets the weak pointer that's associated with this instance.
    * Use this if later on you need to cast sender() into a QSharedPointer
    *
    * @code
    *   QWeakPointer<CalendarBase> weakPtr = qobject_cast<CalendarBase*>( sender() )->weakPointer();
    *   CalendarBase::Ptr calendar( weakPtr.toStrongRef() );
    * @endcode
    *
    * @see weakPointer()
    */
  void setWeakPointer( const QWeakPointer<Akonadi::CalendarBase> &pointer );

  /**
    * Returns the weak pointer set with setWeakPointer().
    * The default is an invalid weak pointer.
    * @see setWeakPointer()
    */
  QWeakPointer<CalendarBase> weakPointer() const;

  /**
    * Adds an Event to the calendar.
    * It's added to akonadi in the background @see createFinished().
    * @param event the event to be added
    */
  /**reimp*/ bool addEvent( const KCalCore::Event::Ptr &event );

  /**
    * Deletes an Event from the calendar.
    * It's removed from akonadi in the background @see deleteFinished().
    * @param event the event to be deleted
    */
  /**reimp*/ bool deleteEvent( const KCalCore::Event::Ptr &event );

  /**
    * Reimplementation of KCalCore::Calendar::deleteAllEvents() that
    * does nothing. Do not use this.
    *
    * A convenience function that makes it easy to have a massive data-loss
    * is a bad idea.
    *
    * @deprecated
    */
  /**reimp*/ void deleteAllEvents();

  /**
    * Adds a Todo to the calendar.
    * It's added to akonadi in the background @see createFinished().
    * @param todo the todo to add
    */
  /**reimp*/ bool addTodo( const KCalCore::Todo::Ptr &todo );

  /**
    * Deletes a Todo from the calendar.
    * It's removed from akonadi in the background @see deleteFinished().
    * @param todo the todo to delete
    */
  /**reimp*/ bool deleteTodo( const KCalCore::Todo::Ptr &todo );

  /**
    * Reimplementation of KCalCore::Calendar::deleteAllTodos() that
    * does nothing. Do not use this.
    *
    * A convenience function that makes it easy to have a massive data-loss
    * is a bad idea.
    *
    * @deprecated
    */
  /**reimp*/ void deleteAllTodos();

  /**
    * Adds a Journal to the calendar.
    * It's added to akonadi in the background @see createFinished().
    * @param journal the journal to add
    */
  /**reimp*/ bool addJournal( const KCalCore::Journal::Ptr &journal );

  /**
    * Deletes a Journal from the calendar.
    * It's removed from akonadi in the background @see deleteFinished().
    * @param journal the journal to delete
    */
  /**reimp*/ bool deleteJournal( const KCalCore::Journal::Ptr &journal );

  /**
    * Reimplementation of KCalCore::Calendar::deleteAllJournals() that
    * does nothing. Do not use this.
    *
    * A convenience function that makes it easy to have a massive data-loss
    * is a bad idea.
    *
    * @deprecated
    */
  /**reimp*/ void deleteAllJournals();

  /**
    * Adds an incidence to the calendar.
    * It's added to akonadi in the background @see createFinished().
    * @param incidence the incidence to add
    */
  /**reimp*/ bool addIncidence( const KCalCore::Incidence::Ptr &incidence );

  /**
    * Deletes an incidence from the calendar.
    * It's removed from akonadi in the background @see deleteFinished().
    * @param incidence the incidence to delete
    */
  /**reimp*/ bool deleteIncidence( const KCalCore::Incidence::Ptr & );

  /**
      Call this to tell the calendar that you're adding a batch of incidences.
      So it doesn't, for example, ask the destination for each incidence.

      @see endBatchAdding()
  */
  /**reimp*/ void startBatchAdding();

  /**
    * Tells the Calendar that you stoped adding a batch of incidences.
    * @see startBatchAdding()
    */
  /**reimp*/ void endBatchAdding();

  /**
    * Returns the IncidenceChanger used by this calendar to make changes in akonadi.
    * Use this if you need the defaults used by CalendarBase.
    */
  Akonadi::IncidenceChanger *incidenceChanger() const;

  /**
    * Modifies an incidence.
    * The incidence with the same uid as @p newIncidence will be updated with the contents of
    * @param newIncidence the incidence to modify
    */
  bool modifyIncidence( const KCalCore::Incidence::Ptr &newIncidence );

Q_SIGNALS:
  /**
    * This signal is emitted when an incidence is created in akonadi through
    * add{Incidence,Event,Todo,Journal}
    * @param success the success of the operation
    * @param errorMessage if @p success is false, contains the error message
    */
  void createFinished( bool success, const QString &errorMessage );

  /**
    * This signal is emitted when an incidence is deleted in akonadi through
    * delete{Incidence,Event,Todo,Journal} or deleteAll{Events,Todos,Journals}
    * @param success the success of the operation
    * @param errorMessage if @p success is false, contains the error message
    */
  void deleteFinished( bool success, const QString &errorMessage );

  /**
    * This signal is emitted when an incidence is modified in akonadi through
    * modifyIncidence().
    * @param success the success of the operation
    * @param errorMessage if @p success is false, contains the error message
    */
  void modifyFinished( bool success, const QString &errorMessage );

protected:
  Q_DECLARE_PRIVATE( CalendarBase )
  QScopedPointer<CalendarBasePrivate> d_ptr;
  CalendarBase( CalendarBasePrivate *const d, QObject *parent );
};
}

#endif
