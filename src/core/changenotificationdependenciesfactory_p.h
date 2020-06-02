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

#ifndef CHANGENOTIFICATIONDEPENDENCIESFACTORY_P_H
#define CHANGENOTIFICATIONDEPENDENCIESFACTORY_P_H

#include "session.h"
#include "entitycache_p.h"

namespace Akonadi
{

class Connection;
class CommandBuffer;

/**
 * This class exists so that we can create a fake notification source in
 * unit tests.
 */
class AKONADI_TESTS_EXPORT ChangeNotificationDependenciesFactory
{
public:
    explicit ChangeNotificationDependenciesFactory() = default;
    virtual ~ChangeNotificationDependenciesFactory() = default;

    virtual Connection *createNotificationConnection(Session *parent, CommandBuffer *commandBuffer);
    virtual void destroyNotificationConnection(Session *parent, Connection *connection);

    virtual QObject *createChangeMediator(QObject *parent);

    virtual Akonadi::CollectionCache *createCollectionCache(int maxCapacity, Session *session);
    virtual Akonadi::ItemCache *createItemCache(int maxCapacity, Session *session);
    virtual Akonadi::ItemListCache *createItemListCache(int maxCapacity, Session *session);
    virtual Akonadi::TagListCache *createTagListCache(int maxCapacity, Session *session);

protected:
    Q_DISABLE_COPY_MOVE(ChangeNotificationDependenciesFactory)

    void addConnection(Session *session, Connection *connection);
};

}

#endif
