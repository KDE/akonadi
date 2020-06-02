/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

#include "changenotification.h"
#include "private/protocol_p.h"

using namespace Akonadi;

namespace Akonadi
{

class AKONADICORE_NO_EXPORT ChangeNotification::Private : public QSharedData
{
public:
    QDateTime timestamp;
    QVector<QByteArray> listeners;
    Protocol::ChangeNotificationPtr notification;
    ChangeNotification::Type type;
};

} // namespace Akonadi

ChangeNotification::ChangeNotification()
    : d(new Private)
{
}

ChangeNotification::ChangeNotification(const ChangeNotification &other)
    : d(other.d)
{
}

ChangeNotification::~ChangeNotification()
{
}

ChangeNotification &ChangeNotification::operator=(const ChangeNotification &other)
{
    d = other.d;
    return *this;
}

bool ChangeNotification::isValid() const
{
    return d->timestamp.isValid();
}

void ChangeNotification::setType(ChangeNotification::Type type)
{
    d->type = type;
}

ChangeNotification::Type ChangeNotification::type() const
{
    return d->type;
}

void ChangeNotification::setListeners(const QVector<QByteArray> &listeners)
{
    d->listeners = listeners;
}

QVector<QByteArray> ChangeNotification::listeners() const
{
    return d->listeners;
}

void ChangeNotification::setTimestamp(const QDateTime &timestamp)
{
    d->timestamp = timestamp;
}

QDateTime ChangeNotification::timestamp() const
{
    return d->timestamp;
}

Protocol::ChangeNotificationPtr ChangeNotification::notification() const
{
    return d->notification;
}

void ChangeNotification::setNotification(const Protocol::ChangeNotificationPtr &ntf)
{
    d->notification = ntf;
}

