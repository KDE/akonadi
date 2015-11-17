/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Michael Jansen <kde@michael-jansen>

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

#include "notificationmanager.h"
#include "notificationmanageradaptor.h"
#include "notificationsource.h"
#include "tracer.h"
#include "storage/datastore.h"
#include "connection.h"

#include <shared/akdebug.h>
#include <private/standarddirs_p.h>
#include <private/xdgbasedirs_p.h>

#include <QtCore/QDebug>
#include <QDBusConnection>
#include <QSettings>

using namespace Akonadi;
using namespace Akonadi::Server;

NotificationManager *NotificationManager::mSelf = 0;

Q_DECLARE_METATYPE(QVector<qint64>)

NotificationManager::NotificationManager()
    : QObject(0)
    , mDebug(false)
{
    qRegisterMetaType<QVector<QByteArray>>();
    qDBusRegisterMetaType<QVector<QByteArray>>();
    qRegisterMetaType<Protocol::ChangeNotification::Type>();
    qDBusRegisterMetaType<Protocol::ChangeNotification::Type>();
    qRegisterMetaType<QVector<qint64>>();
    qDBusRegisterMetaType<QVector<qint64>>();

    new NotificationManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/notifications"),
                                                 this, QDBusConnection::ExportAdaptors);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/notifications/debug"),
                                                 this, QDBusConnection::ExportScriptableSlots |
                                                       QDBusConnection::ExportScriptableSignals);

    const QString serverConfigFile = StandardDirs::serverConfigFile(XdgBaseDirs::ReadWrite);
    QSettings settings(serverConfigFile, QSettings::IniFormat);

    mTimer.setInterval(settings.value(QStringLiteral("NotificationManager/Interval"), 50).toInt());
    mTimer.setSingleShot(true);
    connect(&mTimer, &QTimer::timeout, this, &NotificationManager::emitPendingNotifications);
}

NotificationManager::~NotificationManager()
{
}

NotificationManager *NotificationManager::self()
{
    if (!mSelf) {
        mSelf = new NotificationManager();
    }

    return mSelf;
}

void NotificationManager::connectNotificationCollector(NotificationCollector *collector)
{
    connect(collector, &NotificationCollector::notify,
            this, &NotificationManager::slotNotify);
}

void NotificationManager::registerConnection(Connection *connection)
{
    QMutexLocker locker(&mSourcesLock);
    auto source = std::find_if(mNotificationSources.cbegin(), mNotificationSources.cend(),
                               [connection](NotificationSource *source) {
                                    return connection->sessionId() == source->dbusPath().path().toLatin1();
                               });
    if (source == mNotificationSources.cend()) {
        qWarning() << "Received request to register Notification bus connection, but there's no such subscriber";
        return;
    }

    connect(const_cast<NotificationSource*>(*source), &NotificationSource::notify,
            connection, static_cast<void(Connection::*)(const Protocol::Command &)>(&Connection::sendResponse),
            Qt::QueuedConnection);
}

void NotificationManager::unregisterConnection(Connection *connection)
{
    QMutexLocker locker(&mSourcesLock);
    auto source = std::find_if(mNotificationSources.cbegin(), mNotificationSources.cend(),
                               [connection](NotificationSource *source) {
                                    return connection->sessionId() == source->dbusPath().path().toLatin1();
                               });
    if (source != mNotificationSources.cend()) {
        (*source)->disconnect(connection);
    }
}



void NotificationManager::slotNotify(const Akonadi::Protocol::ChangeNotification::List &msgs)
{
    //akDebug() << Q_FUNC_INFO << "Appending" << msgs.count() << "notifications to current list of " << mNotifications.count() << "notifications";
    Q_FOREACH (const Protocol::ChangeNotification &msg, msgs) {
        Protocol::ChangeNotification::appendAndCompress(mNotifications, msg);
    }
    //akDebug() << Q_FUNC_INFO << "We have" << mNotifications.count() << "notifications queued in total after appendAndCompress()";

    if (!mTimer.isActive()) {
        mTimer.start();
    }
}

void NotificationManager::emitPendingNotifications()
{
    if (mNotifications.isEmpty()) {
        return;
    }

    if (mDebug) {
        QVector<QByteArray> bas;
        bas.reserve(mNotifications.size());
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        Q_FOREACH (const Protocol::ChangeNotification &notification, mNotifications) {
            Tracer::self()->signal("NotificationManager::notify", notification.debugString());
            Protocol::serialize(&buffer, notification);
            bas << buffer.data();
            buffer.buffer().clear();
            buffer.seek(0);
        }
        Q_EMIT debugNotify(bas);
    } else {
        Q_FOREACH (const Protocol::ChangeNotification &notification, mNotifications) {
            Tracer::self()->signal("NotificationManager::notify", notification.debugString());
        }
    }

    Q_FOREACH (NotificationSource *source, mNotificationSources) {
        Protocol::ChangeNotification::List acceptedNotifications;
        Q_FOREACH (const Protocol::ChangeNotification &notification, mNotifications) {
            if (source->acceptsNotification(notification)) {
                acceptedNotifications << notification;
            }
        }

        if (!acceptedNotifications.isEmpty()) {
            source->emitNotification(acceptedNotifications);
        }
    }

    mNotifications.clear();
}

QDBusObjectPath NotificationManager::subscribe(const QString &identifier, bool exclusive)
{
    akDebug() << Q_FUNC_INFO << this << identifier <<  exclusive;
    NotificationSource *source = mNotificationSources.value(identifier);
    if (source) {
        akDebug() << "Known subscriber" << identifier << "subscribes again";
        source->addClientServiceName(message().service());
    } else {
        source = new NotificationSource(identifier, message().service(), this);
    }

    registerSource(source);
    source->setExclusive(exclusive);

    // FIXME KF5: Emit the QDBusObjectPath instead of the identifier
    Q_EMIT subscribed(source->dbusPath());

    return source->dbusPath();
}

void NotificationManager::registerSource(NotificationSource *source)
{
    // Protect write operations because of registerConnection()
    QMutexLocker locker(&mSourcesLock);
    mNotificationSources.insert(source->identifier(), source);
}

void NotificationManager::unsubscribe(const QString &identifier)
{
    NotificationSource *source = mNotificationSources.value(identifier);
    if (source) {
        unregisterSource(source);
        source->deleteLater();
        Q_EMIT unsubscribed(source->dbusPath());
    } else {
        akDebug() << "Attempt to unsubscribe unknown subscriber" << identifier;
    }
}

void NotificationManager::unregisterSource(NotificationSource *source)
{
    // Protect write operations because of registerConnection()
    QMutexLocker locker(&mSourcesLock);
    mNotificationSources.remove(source->identifier());
}

QList<QDBusObjectPath> NotificationManager::subscribers() const
{
    QList<QDBusObjectPath> identifiers;
    identifiers.reserve(mNotificationSources.count());
    Q_FOREACH (NotificationSource *source, mNotificationSources) {
        identifiers << source->dbusPath();
    }

    return identifiers;
}

void NotificationManager::enableDebug(bool enable)
{
    mDebug = enable;
}

bool NotificationManager::debugEnabled() const
{
    return mDebug;
}
