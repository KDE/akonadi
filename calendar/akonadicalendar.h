/*
   Copyright (C) 2011 Sérgio Martins <sergio.martins@kdab.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _AKONADI_AKONADICALENDAR_H_
#define _AKONADI_AKONADICALENDAR_H_

#include "akonadi-calendar_export.h"

#include <Akonadi/Item>
#include <KCalCore/MemoryCalendar>
#include <KCalCore/Incidence>

#include <KDateTime>

namespace Akonadi {
  class AkonadiCalendarPrivate;

  /**
  * @short 
  * AkonadiCalendar is the base class for all akonadi aware calendars.
  *
  * Because it inherits KCalCore::Calendar, it provides seamless integration
  * with the KCalCore and KCalUtils libraries eliminating any need for
  * adapter ( akonadi<->KCalCore ) classes.
  *
  * AkonadiCalendar can not be instantiated directly.
  * @see ETMCalendar
  * @see FetchJobCalendar
  *
  * @author Sérgio Martins <sergio.martins@kdab.com>
  * @since 4.9
  */
  class AKONADI_CALENDAR_EXPORT AkonadiCalendar : public KCalCore::MemoryCalendar
  {
  Q_OBJECT
  public:
    typedef QSharedPointer<AkonadiCalendar> Ptr;

    /**
     * Destroys the calendar.
     */
    ~AkonadiCalendar();

    /**
     * Returns the Item containing the incidence with uid @p uid or an invalid Item
     * if the incidence isn't found.
     */
    Akonadi::Item item( const QString &uid ) const;

    /**
     * Returns the Item with @p id or an invalid Item if not found.
     */    
    Akonadi::Item item( Akonadi::Item::Id ) const;

    /**
     * Returns the child incidences of @p parent.
     * Only the direct childs are returned
     */
    KCalCore::Incidence::List childIncidences( const KCalCore::Incidence::Ptr &parent ) const;

    /**
     * Returns the child incidences of @p parent.
     * Only the direct childs are returned
     */    
    KCalCore::Incidence::List childIncidences( const Akonadi::Item &parent ) const;

    /**
     * Sets the weak pointer that's associated with this instance.
     * Use this if later on you need to cast sender() into a QSharedPointer
     *
     * @code
     *   QWeakPointer<AkonadiCalendar> weakPtr = qobject_cast<AkonadiCalendar*>( sender() )->weakPointer();
     *   AkonadiCalendar::Ptr calendar( weakPtr.toStrongRef() );
     * @endcode
     *
     * @see weakPointer()
     */
    void setWeakPointer( const QWeakPointer<Akonadi::AkonadiCalendar> &pointer );

    /**
     * Returns the weak pointer set with setWeakPointer().
     * The default is an invalid weak pointer.
     * @see setWeakPointer()
     */
    QWeakPointer<AkonadiCalendar> weakPointer() const;

    /**
     * Adds an Event to the calendar.
     * It's added to akonadi in the background @see createFinished().
     */
    /**reimp*/ bool addEvent( const KCalCore::Event::Ptr &event );

    /**
     * Deletes an Event from the calendar.
     * It's removed from akonadi in the background @see deleteFinished().
     */  
    /**reimp*/ bool deleteEvent( const KCalCore::Event::Ptr &event );

    /**
     * Deletes all Events from the calendar.
     * They are removed from akonadi in the background @see deleteFinished().
     */   
    /**reimp*/ void deleteAllEvents();

    /**
     * Adds a Todo to the calendar.
     * It's added to akonadi in the background @see createFinished().
     */
    /**reimp*/ bool addTodo( const KCalCore::Todo::Ptr &todo );

    /**
     * Deletes a Todo from the calendar.
     * It's removed from akonadi in the background @see deleteFinished().
     */   
    /**reimp*/ bool deleteTodo( const KCalCore::Todo::Ptr &todo );

    /**
     * Deletes all Todos from the calendar.
     * They are removed from akonadi in the background @see deleteFinished().
     */    
    /**reimp*/ void deleteAllTodos();

    /**
     * Adds a Journal to the calendar.
     * It's added to akonadi in the background @see createFinished().
     */  
    /**reimp*/ bool addJournal( const KCalCore::Journal::Ptr &journal );

    /**
     * Deletes a Journal from the calendar.
     * It's removed from akonadi in the background @see deleteFinished().
     */     
    /**reimp*/ bool deleteJournal( const KCalCore::Journal::Ptr &journal );

    /**
     * Deletes all Journals from the calendar.
     * They are removed from akonadi in the background @see deleteFinished().
     */  
    /**reimp*/ void deleteAllJournals();

    /**
     * Adds an incidence to the calendar.
     * It's added to akonadi in the background @see createFinished().
     */ 
    /**reimp*/ bool addIncidence( const KCalCore::Incidence::Ptr &incidence );

    /**
     * Deletes an incidence from the calendar.
     * It's removed from akonadi in the background @see deleteFinished().
     */    
    /**reimp*/ bool deleteIncidence( const KCalCore::Incidence::Ptr & );

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

  protected:
    Q_DECLARE_PRIVATE( AkonadiCalendar );
    QScopedPointer<AkonadiCalendarPrivate> d_ptr;
    explicit AkonadiCalendar( const KDateTime::Spec &timeSpec );
    AkonadiCalendar( AkonadiCalendarPrivate *const d, const KDateTime::Spec &timeSpec );
  };
}

#endif
