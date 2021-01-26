/*
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "notificationsource_p.h"
#include "notificationsourceinterface.h"

using namespace Akonadi;

NotificationSource::NotificationSource(QObject *source)
    : QObject(source)
{
    Q_ASSERT(source);
}

NotificationSource::~NotificationSource()
{
}

QString NotificationSource::identifier() const
{
    auto *source = qobject_cast<org::freedesktop::Akonadi::NotificationSource *>(parent());
    return source->path();
}

void NotificationSource::setAllMonitored(bool allMonitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setAllMonitored", Q_ARG(bool, allMonitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setExclusive(bool exclusive)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setExclusive", Q_ARG(bool, exclusive));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setMonitoredCollection(Collection::Id id, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredCollection", Q_ARG(qlonglong, id), Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setMonitoredItem(Item::Id id, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredItem", Q_ARG(qlonglong, id), Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setMonitoredResource(const QByteArray &resource, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredResource", Q_ARG(QByteArray, resource), Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setMonitoredMimeType(const QString &mimeType, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredMimeType", Q_ARG(QString, mimeType), Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setIgnoredSession(const QByteArray &session, bool ignored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setIgnoredSession", Q_ARG(QByteArray, session), Q_ARG(bool, ignored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setMonitoredTag(Tag::Id id, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredTag", Q_ARG(qlonglong, id), Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setMonitoredType(Protocol::ChangeNotification::Type type, bool monitored)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setMonitoredType", Q_ARG(Akonadi::Protocol::ChangeNotification::Type, type), Q_ARG(bool, monitored));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

void NotificationSource::setSession(const QByteArray &session)
{
    const bool ok = QMetaObject::invokeMethod(parent(), "setSession", Q_ARG(QByteArray, session));
    Q_ASSERT(ok);
    Q_UNUSED(ok)
}

QObject *NotificationSource::source() const
{
    return parent();
}
