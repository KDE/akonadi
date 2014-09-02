/*
  Copyright (c) 2011-2012 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef AKONADI_FETCHJOBCALENDAR_P_H
#define AKONADI_FETCHJOBCALENDAR_P_H

#include "fetchjobcalendar.h"
#include "calendarbase_p.h"


class KJob;

namespace Akonadi {

class FetchJobCalendarPrivate : public CalendarBasePrivate
{
    Q_OBJECT
public:

    explicit FetchJobCalendarPrivate(FetchJobCalendar *qq);
    ~FetchJobCalendarPrivate();

public Q_SLOTS:
    void slotSearchJobFinished(KJob *job);
    void slotFetchJobFinished();

public:
    bool m_isLoaded;

private:
    FetchJobCalendar *const q;
    QString mErrorMessage;
    bool mSuccess;
};

}

#endif
