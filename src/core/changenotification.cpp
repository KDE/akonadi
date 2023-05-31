/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changenotification.h"
#include "private/protocol_p.h"

using namespace Akonadi;

namespace Akonadi
{
class ChangeNotificationPrivate : public QSharedData
{
public:
    QDateTime timestamp;
    QList<QByteArray> listeners;
    Protocol::ChangeNotificationPtr notification;
    ChangeNotification::Type type;
};

} // namespace Akonadi

ChangeNotification::ChangeNotification()
    : d(new ChangeNotificationPrivate)
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

void ChangeNotification::setListeners(const QList<QByteArray> &listeners)
{
    d->listeners = listeners;
}

QList<QByteArray> ChangeNotification::listeners() const
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
