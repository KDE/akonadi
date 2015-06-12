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

#include <shared/akdebug.h>
#include <shared/akstandarddirs.h>
#include <private/xdgbasedirs_p.h>

#include <QtCore/QDebug>
#include <QDBusConnection>
#include <QSettings>

using namespace Akonadi;
using namespace Akonadi::Server;

NotificationManager *NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
    : QObject(0)
{
    NotificationMessage::registerDBusTypes();
    NotificationMessageV2::registerDBusTypes();
    NotificationMessageV3::registerDBusTypes();

    new NotificationManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/notifications"),
                                                 this, QDBusConnection::ExportAdaptors);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/notifications/debug"),
                                                 this, QDBusConnection::ExportScriptableSlots);

    const QString serverConfigFile = AkStandardDirs::serverConfigFile(XdgBaseDirs::ReadWrite);
    QSettings settings(serverConfigFile, QSettings::IniFormat);

    mTimer.setInterval(settings.value(QLatin1String("NotificationManager/Interval"), 50).toInt());
    mTimer.setSingleShot(true);
    connect(&mTimer, SIGNAL(timeout()), SLOT(emitPendingNotifications()));
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
    connect(collector, SIGNAL(notify(Akonadi::NotificationMessageV3::List)),
            SLOT(slotNotify(Akonadi::NotificationMessageV3::List)));
}

void NotificationManager::slotNotify(const Akonadi::NotificationMessageV3::List &msgs)
{
    //akDebug() << Q_FUNC_INFO << "Appending" << msgs.count() << "notifications to current list of " << mNotifications.count() << "notifications";
    Q_FOREACH (const NotificationMessageV3 &msg, msgs) {
        NotificationMessageV3::appendAndCompress(mNotifications, msg);
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

    NotificationMessage::List legacyNotifications;
    Q_FOREACH (const NotificationMessageV3 &notification, mNotifications) {
        Tracer::self()->signal("NotificationManager::notify", notification.toString());
    }

    if (!legacyNotifications.isEmpty()) {
        Q_FOREACH (NotificationSource *src, mNotificationSources) {
            src->emitNotification(legacyNotifications);
        }
    }

    Q_FOREACH (NotificationSource *source, mNotificationSources) {
        if (!source->isServerSideMonitorEnabled()) {
            source->emitNotification(mNotifications);
            continue;
        }

        NotificationMessageV3::List acceptedNotifications;
        Q_FOREACH (const NotificationMessageV3 &notification, mNotifications) {
            if (source->acceptsNotification(notification)) {
                acceptedNotifications << notification;
            }
        }

        if (!acceptedNotifications.isEmpty()) {
            source->emitNotification(acceptedNotifications);
        }
    }


    // backward compatibility with the old non-subcription interface
    // FIXME: Can we drop this already?
    if (!legacyNotifications.isEmpty()) {
        Q_EMIT notify(legacyNotifications);
    }

    mNotifications.clear();
}

QDBusObjectPath NotificationManager::subscribeV2(const QString &identifier, bool serverSideMonitor)
{
    akDebug() << Q_FUNC_INFO << this << identifier << serverSideMonitor;
    return subscribeV3(identifier, serverSideMonitor, false);
}

QDBusObjectPath NotificationManager::subscribeV3(const QString &identifier, bool serverSideMonitor, bool exclusive)
{
    akDebug() << Q_FUNC_INFO << this << identifier << serverSideMonitor << exclusive;

    NotificationSource *source = mNotificationSources.value(identifier);
    if (source) {
        akDebug() << "Known subscriber" << identifier << "subscribes again";
        source->addClientServiceName(message().service());
    } else {
        source = new NotificationSource(identifier, message().service(), this);
    }

    registerSource(source);
    source->setServerSideMonitorEnabled(serverSideMonitor);
    source->setExclusive(exclusive);

    // The path is /subscriber/escaped_identifier. We want to extract
    // the escaped_identifier and emit it in subscribed() instead of the original
    // identifier
    const QStringList paths = source->dbusPath().path().split(QLatin1Char('/'), QString::SkipEmptyParts);

    // FIXME KF5: Emit the QDBusObjectPath instead of the identifier
    Q_EMIT subscribed(paths.at(1));

    return source->dbusPath();
}

void NotificationManager::registerSource(NotificationSource *source)
{
    mNotificationSources.insert(source->identifier(), source);
}

QDBusObjectPath NotificationManager::subscribe(const QString &identifier)
{
    akDebug() << Q_FUNC_INFO << this << identifier;
    return subscribeV2(identifier, false);
}

void NotificationManager::unsubscribe(const QString &identifier)
{
    NotificationSource *source = mNotificationSources.value(identifier);
    if (source) {
        unregisterSource(source);
        const QStringList paths = source->dbusPath().path().split(QLatin1Char('/'), QString::SkipEmptyParts);
        source->deleteLater();
        Q_EMIT unsubscribed(paths.at(1));
    } else {
        akDebug() << "Attempt to unsubscribe unknown subscriber" << identifier;
    }
}

void NotificationManager::unregisterSource(NotificationSource *source)
{
    mNotificationSources.remove(source->identifier());
}

QStringList NotificationManager::subscribers() const
{
    QStringList identifiers;
    Q_FOREACH (NotificationSource *source, mNotificationSources) {
        identifiers << source->identifier();
    }

    return identifiers;
}
