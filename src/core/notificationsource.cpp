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

#include "notificationsource_p.h"

using namespace Akonadi;

NotificationSource::NotificationSource(QObject *source)
    : QObject(source)
{
    Q_ASSERT(source);

    connect(source, SIGNAL(notifyV3(Akonadi::NotificationMessageV3::List)),
            this, SIGNAL(notifyV3(Akonadi::NotificationMessageV3::List)));
}

NotificationSource::~NotificationSource()
{
}

void NotificationSource::setAllMonitored(bool allMonitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setAllMonitored",
                                              Q_ARG(bool, allMonitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setExclusive(bool exclusive)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setExclusive",
                                              Q_ARG(bool, exclusive));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setMonitoredCollection(Entity::Id id, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredCollection",
                                              Q_ARG(qlonglong, id),
                                              Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setMonitoredItem(Entity::Id id, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredItem",
                                              Q_ARG(qlonglong, id),
                                              Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setMonitoredResource(const QByteArray &resource, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredResource",
                                              Q_ARG(QByteArray, resource),
                                              Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setMonitoredMimeType(const QString &mimeType, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredMimeType",
                                              Q_ARG(QString, mimeType),
                                              Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setIgnoredSession(const QByteArray &session, bool ignored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setIgnoredSession",
                                              Q_ARG(QByteArray, session),
                                              Q_ARG(bool, ignored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setMonitoredTag(Tag::Id id, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredTag",
                                              Q_ARG(qlonglong, id),
                                              Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setMonitoredType(NotificationMessageV2::Type type, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredType",
                                              Q_ARG(Akonadi::NotificationMessageV2::Type, type),
                                              Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

void NotificationSource::setSession(const QByteArray &session)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setSession",
                                              Q_ARG(QByteArray, session));
    Q_ASSERT(ok);
    Q_UNUSED(ok);
}

QObject *NotificationSource::source() const
{
    return parent();
}
