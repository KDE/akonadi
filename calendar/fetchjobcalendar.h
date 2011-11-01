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

#ifndef _AKONADI_FETCHJOBCALENDAR_H_
#define _AKONADI_FETCHJOBCALENDAR_H_

#include "akonadi-calendar_export.h"
#include "akonadicalendar.h"

#include <Akonadi/Item>
#include <KDateTime>
#include <KSystemTimeZones>

namespace Akonadi {
  class FetchJobCalendarPrivate;
  /**
  * @short
  * An AkonadiCalendar that gets populated by a one time IncidenceFetchJob.
  *
  * If you want a persistent calendar ( which monitors Akonadi for changes )
  * use an ETMCalendar.
  * 
  * @see ETMCalendar
  * @see AkonadiCalendar
  *
  * @author Sérgio Martins <sergio.martins@kdab.com>
  * @since 4.9
  */
  class AKONADI_CALENDAR_EXPORT FetchJobCalendar : public Akonadi::AkonadiCalendar
  {
  Q_OBJECT
  public:
    typedef QSharedPointer<FetchJobCalendar> Ptr;

    /**
     * Creates a new FetchJobCalendar. Loading begins asynchronously.
     * @see loadFinished()
     *///TODO: timeSpec
    explicit FetchJobCalendar( const KDateTime::Spec &timeSpec = KSystemTimeZones::local());

    /**
     * Destroys this FetchJobCalendar.
     */
    ~FetchJobCalendar();

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
