/*
  Copyright (C) 2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (C) 2010-2012 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef AKONADI_CALENDAR_UTILS_P_
#define AKONADI_CALENDAR_UTILS_P_

#include <kcalcore/incidence.h>
#include <akonadi/item.h>

#include <QString>
#include <QStringList>

/**
 * Here lives a bunch of code, copied mainly from kdepim/calendarsupport/ and mostly
 * functions that existed in kcalprefs.cpp to manage identity stuff.
 *
 * These functions are to be phased out in favour of KPIMIdentities.
 */

namespace Akonadi {
  namespace CalendarUtils {
    QString fullName();
    QString email();
    bool thatIsMe( const QString &email );
    QStringList allEmails();

    KCalCore::Incidence::Ptr incidence( const Akonadi::Item &item );
  }
}

#endif
