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
#include "sessionthread_p.h"
#include "connection_p.h"
#include "changemediator_p.h"
#include "servermanager.h"
#include "akonadicore_debug.h"
#include "session_p.h"

#include <KRandom>
#include <qdbusextratypes.h>

using namespace Akonadi;

Connection *ChangeNotificationDependenciesFactory::createNotificationConnection(Session *session)
{
    if (!Akonadi::ServerManager::self()->isRunning()) {
        return Q_NULLPTR;
    }

    return session->d->sessionThread()->createConnection(Connection::NotificationConnection, session->sessionId());
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
