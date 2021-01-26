/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CHANGENOTIFICATIONDEPENDENCIESFACTORY_P_H
#define CHANGENOTIFICATIONDEPENDENCIESFACTORY_P_H

#include "entitycache_p.h"
#include "session.h"

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
