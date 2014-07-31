/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_NOTIFICATIONMESSAGEV3_H
#define AKONADI_NOTIFICATIONMESSAGEV3_H

#include "akonadiprotocolinternals_export.h"

#include "notificationmessagev2_p.h"
#include <QDBusArgument>
#include <QDebug>

namespace Akonadi {

class AKONADIPROTOCOLINTERNALS_EXPORT NotificationMessageV3 : public Akonadi::NotificationMessageV2
{
public:
    typedef QVector<NotificationMessageV3> List;

    static void registerDBusTypes();

    NotificationMessageV3();
    NotificationMessageV3(const NotificationMessageV3 &other);
    ~NotificationMessageV3();

    static NotificationMessageV2::List toV2List(const NotificationMessageV3::List &list);
    static bool appendAndCompress(NotificationMessageV3::List &list, const NotificationMessageV3 &msg);
    static bool appendAndCompress(QList<NotificationMessageV3> &list, const NotificationMessageV3 &msg);

};

}

AKONADIPROTOCOLINTERNALS_EXPORT QDebug operator<<(QDebug dbg, const Akonadi::NotificationMessageV3 &msg);

const QDBusArgument &operator>>(const QDBusArgument &arg, Akonadi::NotificationMessageV3 &msg);
QDBusArgument &operator<<(QDBusArgument &arg, const Akonadi::NotificationMessageV3 &msg);

Q_DECLARE_TYPEINFO(Akonadi::NotificationMessageV3, Q_MOVABLE_TYPE);
Q_DECLARE_METATYPE(Akonadi::NotificationMessageV3)
Q_DECLARE_METATYPE(Akonadi::NotificationMessageV3::List)

#endif // AKONADI_NOTIFICATIONMESSAGEV3_H
