/*
    Copyright (c) 2014 Christian Mollekopf <mollekopf@kolabsys.com>

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
#include "virtualresource.h"

#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/resourceselectjob_p.h>
#include <akonadi/servermanager.h>
#include <akonadi/session_p.h>
#include <qdbusinterface.h>
#include <dbusconnectionpool.h>
#include <QStringList>

#define EXEC(job) \
do { \
  if (!job->exec()) { \
      kFatal() << "Job failed: " << job->errorString(); \
  } \
} while ( 0 )

using namespace Akonadi;

VirtualResource::VirtualResource(const QString &name, QObject *parent)
    : QObject(parent),
    mResourceName(name)
{
    // QDBusInterface *interface = new QDBusInterface(ServerManager::serviceName(ServerManager::Control),
    //                                QString::fromLatin1("/"),
    //                                QString::fromLatin1("org.freedesktop.Akonadi.AgentManager"),
    //                                DBusConnectionPool::threadConnection(), this);
    // if (interface->isValid()) {
    //     const QDBusMessage reply = interface->call(QString::fromUtf8("createAgentInstance"), name, QStringList());
    //     if (reply.type() == QDBusMessage::ErrorMessage) {
    //         // This means that the resource doesn't provide a synchronizeCollectionAttributes method, so we just finish the job
    //         return;
    //     }
    // } else {
    //     Q_ASSERT(false);
    // }
    // mSession = new Akonadi::Session(name.toLatin1(), this);

    // Since this is in the same process as the test, all jobs in the test get executed in the resource session by default
    SessionPrivate::createDefaultSession(name.toLatin1());
    mSession = Session::defaultSession();
    ResourceSelectJob *select = new ResourceSelectJob(name, mSession);
    EXEC(select);
}

VirtualResource::~VirtualResource()
{
    if (mRootCollection.isValid()) {
        CollectionDeleteJob *d = new CollectionDeleteJob(mRootCollection, mSession);
        EXEC(d);
    }
}

Akonadi::Collection VirtualResource::createCollection(const Akonadi::Collection &collection)
{
    // kDebug() << collection.name() << collection.parentCollection().remoteId();
    // kDebug() << "contentMimeTypes: " << collection.contentMimeTypes();
    
    Q_ASSERT(!collection.name().isEmpty());
    Collection col = collection;
    if (!col.parentCollection().isValid()) {
        col.setParentCollection(mRootCollection);
    }
    CollectionCreateJob *create = new CollectionCreateJob(col, mSession);
    EXEC(create);
    return create->collection();
}
Akonadi::Collection VirtualResource::createRootCollection(const Akonadi::Collection &collection)
{
    kDebug() << collection.name();
    mRootCollection = createCollection(collection);
    return mRootCollection;
}

Akonadi::Item VirtualResource::createItem(const Akonadi::Item &item, const Collection &parent)
{
    ItemCreateJob *create = new ItemCreateJob(item, parent, mSession);
    EXEC(create);
    return create->item();
}

void VirtualResource::reset()
{
    Q_ASSERT(mRootCollection.isValid());
    Akonadi::Collection col = mRootCollection;
    CollectionDeleteJob *d = new CollectionDeleteJob(mRootCollection, mSession);
    EXEC(d);
    col.setId(-1);
    createRootCollection(col);
}

