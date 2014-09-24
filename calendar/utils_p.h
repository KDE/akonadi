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
#include <akonadi/collection.h>

#include <QString>
#include <QStringList>

/**
 * Util functions that have no place to live.
 */

class QWidget;

namespace Akonadi {
namespace CalendarUtils {
QString fullName();
QString email();
bool thatIsMe(const QString &email);

//faster version, because we know that attendee->email() is only the email address
bool thatIsMe(const KCalCore::Attendee::Ptr &attendee);

QStringList allEmails();

KCalCore::Incidence::Ptr incidence(const Akonadi::Item &item);

Akonadi::Collection selectCollection(QWidget *parent,
                                     int &dialogCode,
                                     const QStringList &mimeTypes,
                                     const Akonadi::Collection &defaultCollection = Akonadi::Collection());

}
}

#endif
