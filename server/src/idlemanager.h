/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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


#ifndef AKONADI_IDLEMANAGER_H
#define AKONADI_IDLEMANAGER_H

#include <QObject>
#include <QHash>
#include <QReadWriteLock>

#include "exception.h"
#include <fetchhelper.h>
#include <notificationmessagev2_p.h>
#include <build-devel/server/entities.h>

namespace Akonadi
{

AKONADI_EXCEPTION_MAKE_INSTANCE( IdleException );

class AkonadiConnection;
class IdleClient;

class IdleManager : public QObject
{
    Q_OBJECT

  public:
    static IdleManager *self();

    virtual ~IdleManager();

    IdleClient *clientForConnection( AkonadiConnection *connection );
    void registerClient( IdleClient *client );
    void unregisterClient( IdleClient *client );

    // TODO:
    // So, for now we are using NotificationManager's queue and let it compress
    // notifications for us. In the long term we should probably make it the other way
    // around and have IdleManager to control notifications and NotificationManager
    // to just handle notifications over DBus for backwards compatibility
    void notify( NotificationMessageV2::List &msg );

    void updateGlobalFetchScope( const FetchScope &oldFetchScope, const FetchScope &newFetchScope );

  private:
    explicit IdleManager();
    static IdleManager *s_instance;

    // We don't support batch operations on collections (yet), so msg
    // will always have only one collection
    Collection fetchCollection( const NotificationMessageV2 &msg );

  private Q_SLOTS:
    void fetchHelperResponseAvailable( const Akonadi::Response &response );

  private:
    QHash<QByteArray, IdleClient*> mClientsRegistrar;
    QReadWriteLock mRegistrarLock;

    // Set of all parts requested by all clients
    QStringList mRequestedParts;
    // Whether at least one client requests full payload
    bool mFullPayload;

    FetchScope mFetchScope;
};

}

#endif // AKONADI_IDLEMANAGER_H
