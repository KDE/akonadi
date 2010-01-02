/*
  Copyright (c) 2008-2009 Kevin Krammer <kevin.krammer@gmx.at>

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

#ifndef AKONADI_KCAL_INCIDENCEMIMETYPEVISITOR_H
#define AKONADI_KCAL_INCIDENCEMIMETYPEVISITOR_H

#include "akonadi-kcal_export.h"

#include <kcal/incidencebase.h>

namespace Akonadi {

/**
  Helper for getting the Akonadi specific sub MIME type of a KCal::IncidenceBase
  item, e.g. getting "application/x-vnd.akonadi.calendar.event" for a
  KCal::Event.

  Usage example: creating Akonadi items for a list of incidences
  @code
  KCal::Incidence::List incidences; // assume it is filled somewhere else

  IncidenceMimeTypeVisitor visitor;
  Akonadi::Item::List items;
  foreach ( Incidence *incidence, incidences ) {
    incidence->accept( visitor );

    Akonadi::Item item( visitor.mimeType() );
    item.setPayload<IncidencePtr>( IncidencePtr( incidence->clone() ) );

    items << item;
  }
  @endcode

  @since 4.4
 */
class AKONADI_KCAL_EXPORT IncidenceMimeTypeVisitor : public KCal::IncidenceBase::Visitor
{
  public:
    /**
      Creates a visitor instance.

      Until its first visit mimeType() will return @c QString()
     */
    IncidenceMimeTypeVisitor();

    /**
      Destroys the instance
     */
    virtual ~IncidenceMimeTypeVisitor();

    /**
      Sets the MIME type to "application/x-vnd.akonadi.calendar.event"

      @param event The Event to visit. Not used since the MIME type does not
                   depend on instance specific properties.

      @return always returns @c true
     */
    virtual bool visit( KCal::Event *event );

    /**
      Sets the MIME type to "application/x-vnd.akonadi.calendar.todo"

      @param todo The Todo to visit. Not used since the MIME type does not
                   depend on instance specific properties.

      @return always returns @c true
     */
    virtual bool visit( KCal::Todo *todo );

    /**
      Sets the MIME type to "application/x-vnd.akonadi.calendar.journal"

      @param journal The Journal to visit. Not used since the MIME type does not
                     depend on instance specific properties.

      @return always returns @c true
     */
    virtual bool visit( KCal::Journal *journal );

    /**
      Sets the MIME type to "application/x-vnd.akonadi.calendar.freebusy"

      @param freebusy The FreeBusy to visit. Not used since the MIME type does
                      not depend on instance specific properties.

      @return always returns @c true
     */
    virtual bool visit( KCal::FreeBusy *freebusy );

    /**
      Returns the Akonadi specific @c text/calendar sub MIME type of the last
      incidence visited by this instance.

      @return One of the Akonadi sub MIME types for calendar components or
              @c QString() if no incidence visited yet
     */
    QString mimeType() const;

    /**
      Returns a list of all calendar component sub MIME types.
     */
    QStringList allMimeTypes() const;

    /**
      Returns the Akonadi specific @c text/calendar sub MIME type of the given
      @p incidence.

      This is a convenience method, equivalent to
      @code
      incidence->accept( visitor );
      return visitor.mimeType();
      @endcode
    */
    QString mimeType( KCal::IncidenceBase *incidence );

    /**
      Returns the sub MIME type for Events
    */
    static QString eventMimeType();

    /**
      Returns the sub MIME type for Todos
    */
    static QString todoMimeType();

    /**
      Returns the sub MIME type for Journals
    */
    static QString journalMimeType();

    /**
      Returns the sub MIME type for FreeBusys
    */
    static QString freeBusyMimeType();

  private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond

    Q_DISABLE_COPY( IncidenceMimeTypeVisitor )
};

}

#endif

// kate: space-indent on; indent-width 2; replace-tabs on;
