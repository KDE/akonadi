/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "changemediator_p.h"
#include "changenotificationdependenciesfactory_p.h"
#include "connection_p.h"
#include "servermanager.h"
#include "session_p.h"
#include "sessionthread_p.h"

using namespace Akonadi;

Connection *ChangeNotificationDependenciesFactory::createNotificationConnection(Session *session, CommandBuffer *commandBuffer)
{
    if (!Akonadi::ServerManager::self()->isRunning()) {
        return nullptr;
    }

    auto connection = new Connection(Connection::NotificationConnection, session->sessionId(), commandBuffer);
    addConnection(session, connection);
    return connection;
}

void ChangeNotificationDependenciesFactory::addConnection(Session *session, Connection *connection)
{
    session->d->sessionThread()->addConnection(connection);
}

void ChangeNotificationDependenciesFactory::destroyNotificationConnection(Session *session, Connection *connection)
{
    session->d->sessionThread()->destroyConnection(connection);
}

QObject *ChangeNotificationDependenciesFactory::createChangeMediator(QObject *parent)
{
    Q_UNUSED(parent)
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
