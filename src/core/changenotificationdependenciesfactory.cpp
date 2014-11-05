/*
    Copyright (c) 2011 Stephen Kelly <steveire@gmail.com>

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

#include "changenotificationdependenciesfactory_p.h"
#include "KDBusConnectionPool"
#include "notificationsource_p.h"
#include "notificationsourceinterface.h"
#include "notificationmanagerinterface.h"
#include "changemediator_p.h"
#include "servermanager.h"

#include <KComponentData>
#include <KGlobal>
#include <KRandom>
#include <qdbusextratypes.h>

using namespace Akonadi;

NotificationSource *ChangeNotificationDependenciesFactory::createNotificationSource(QObject *parent)
{
    if (!Akonadi::ServerManager::self()->isRunning()) {
        return 0;
    }

    org::freedesktop::Akonadi::NotificationManager *manager =
        new org::freedesktop::Akonadi::NotificationManager(
        ServerManager::serviceName(Akonadi::ServerManager::Server),
        QStringLiteral("/notifications"),
        KDBusConnectionPool::threadConnection());

    if (!manager) {
        // :TODO: error handling
        return 0;
    }

    const QString name =
        QString::fromLatin1("%1_%2_%3").arg(
            KGlobal::mainComponent().componentName(),
            QString::number(QCoreApplication::applicationPid()),
            KRandom::randomString(6));
    QDBusObjectPath p = manager->subscribeV2(name, true);
    const bool validError = manager->lastError().isValid();
    if (validError) {
        qWarning() << manager->lastError().name() << manager->lastError().message();
        // :TODO: What to do?
        delete manager;
        return 0;
    }
    delete manager;
    org::freedesktop::Akonadi::NotificationSource *notificationSource =
        new org::freedesktop::Akonadi::NotificationSource(
        ServerManager::serviceName(Akonadi::ServerManager::Server),
        p.path(),
        KDBusConnectionPool::threadConnection(), parent);

    if (!notificationSource) {
        // :TODO: error handling
        return 0;
    }
    return new NotificationSource(notificationSource);
}

QObject *ChangeNotificationDependenciesFactory::createChangeMediator(QObject *parent)
{
    Q_UNUSED(parent);
    return ChangeMediator::instance();
}

CollectionCache *ChangeNotificationDependenciesFactory::createCollectionCache(int maxCapacity, Session *session)
{
    return new CollectionCache(maxCapacity, session);
}

ItemCache *ChangeNotificationDependenciesFactory::createItemCache(int maxCapacity, Session *session)
{
    return new ItemCache(maxCapacity, session);
}

ItemListCache *ChangeNotificationDependenciesFactory::createItemListCache(int maxCapacity, Session *session)
{
    return new ItemListCache(maxCapacity, session);
}

TagListCache *ChangeNotificationDependenciesFactory::createTagListCache(int maxCapacity, Session *session)
{
    return new TagListCache(maxCapacity, session);
}
