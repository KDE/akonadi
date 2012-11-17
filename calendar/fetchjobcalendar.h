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

#ifndef _AKONADI_FETCHJOBCALENDAR_H_
#define _AKONADI_FETCHJOBCALENDAR_H_

#include "akonadi-calendar_export.h"
#include "calendarbase.h"

#include <akonadi/item.h>
#include <KDateTime>
#include <KSystemTimeZones>

namespace Akonadi {
class FetchJobCalendarPrivate;
/**
* @short A Calendar that gets populated by a one time IncidenceFetchJob.
*
* If you want a persistent calendar ( which monitors Akonadi for changes )
* use an ETMCalendar.
*
* @see ETMCalendar
* @see CalendarBase
*
* @author Sérgio Martins <sergio.martins@kdab.com>
* @since 4.11
*/
class AKONADI_CALENDAR_EXPORT FetchJobCalendar : public Akonadi::CalendarBase
{
  Q_OBJECT
public:
  typedef QSharedPointer<FetchJobCalendar> Ptr;

  /**
    * Creates a new FetchJobCalendar. Loading begins asynchronously.
    * @see loadFinished()
    */
  explicit FetchJobCalendar();

  /**
    * Destroys this FetchJobCalendar.
    */
  ~FetchJobCalendar();

  /**
    * Returns if the calendar already finished loading.
    * This is an alternative to listening for the loadFinished() signal.
    */
  bool isLoaded() const;

Q_SIGNALS:
  /**
    * This signal is emitted when the IncidenceFetchJob finishes.
    * @param success the success of the operation
    * @param errorMessage if @p success is false, contains the error message
    */
  void loadFinished( bool success, const QString &errorMessage );
};

}

#endif
