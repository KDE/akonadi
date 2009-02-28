/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "itemretrievalmanager.h"

#include "resourceinterface.h"
#include "akdebug.h"

#include <QCoreApplication>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

using namespace Akonadi;

ItemRetrievalManager::Request::Request() :
  processed( false )
{
}


ItemRetrievalManager* ItemRetrievalManager::sInstance = 0;

ItemRetrievalManager::ItemRetrievalManager( QObject *parent ) :
  QObject( parent )
{
  // make sure we are created from the retrieval thread and only once
  Q_ASSERT( QThread::currentThread() != QCoreApplication::instance()->thread() );
  Q_ASSERT( sInstance == 0 );
  sInstance = this;

  mLock = new QReadWriteLock();
  mWaitCondition = new QWaitCondition();

  connect( QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
           this, SLOT(serviceOwnerChanged(QString,QString,QString)) );
  connect( this, SIGNAL(requestAdded()), this, SLOT(processRequest()), Qt::QueuedConnection );
  connect( this, SIGNAL(syncCollection(QString,qint64)), this, SLOT(triggerCollectionSync(QString,qint64)), Qt::QueuedConnection );
}

ItemRetrievalManager::~ItemRetrievalManager()
{
  delete mWaitCondition;
  delete mLock;
}

ItemRetrievalManager* ItemRetrievalManager::instance()
{
  Q_ASSERT( sInstance );
  return sInstance;
}

// called within the retrieval thread
void ItemRetrievalManager::serviceOwnerChanged(const QString& serviceName, const QString& oldOwner, const QString& newOwner)
{
  Q_UNUSED( newOwner );
  if ( oldOwner.isEmpty() )
    return;
  if ( !serviceName.startsWith( QLatin1String("org.freedesktop.Akonadi.Resource.") ) )
    return;
  const QString resourceId = serviceName.mid( 33 );
  qDebug() << "Lost connection to resource" << serviceName << ", discarding cached interface";
  mResourceInterfaces.remove( resourceId );
}

// called within the retrieval thread
OrgFreedesktopAkonadiResourceInterface* ItemRetrievalManager::resourceInterface(const QString& id)
{
  if ( id.isEmpty() )
    return 0;

  OrgFreedesktopAkonadiResourceInterface *iface = 0;
  if ( mResourceInterfaces.contains( id ) )
    iface = mResourceInterfaces.value( id );
  if ( iface && iface->isValid() )
    return iface;

  delete iface;
  iface = new OrgFreedesktopAkonadiResourceInterface( QLatin1String("org.freedesktop.Akonadi.Resource.") + id,
                                                      QLatin1String("/"), QDBusConnection::sessionBus(), this );
  if ( !iface || !iface->isValid() ) {
    qDebug() << QString::fromLatin1( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                                    .arg( id, iface ? iface->lastError().message() : QString() );
    delete iface;
    return 0;
  }
  mResourceInterfaces.insert( id, iface );
  return iface;
}

// called from any thread
void ItemRetrievalManager::requestItemDelivery( qint64 uid, const QByteArray& remoteId, const QByteArray& mimeType,
                                               const QString& resource, const QStringList& parts )
{
  qDebug() << "requestItemDelivery() - current thread:" << QThread::currentThread() << " retrieval thread:" << thread();

  Request *req = new Request();
  req->id = uid;
  req->remoteId = remoteId;
  req->mimeType = mimeType;
  req->resourceId = resource;
  req->parts = parts;

  mLock->lockForWrite();
  qDebug() << "posting retrieval request for item" << uid ;
  mPendingRequests.append( req );
  mLock->unlock();

  emit requestAdded();

  mLock->lockForRead();
  forever {
    qDebug() << "checking if request for item" << uid << "has been processed...";
    if ( req->processed ) {
      Q_ASSERT( !mPendingRequests.contains( req ) );
      const QString errorMsg = req->errorMsg;
      mLock->unlock();
      qDebug() << "request for item" << uid << "processed, error:" << errorMsg;
      delete req;
      if ( errorMsg.isEmpty() )
        return;
      else
        throw ItemRetrieverException( errorMsg );
    } else {
      qDebug() << "request for item" << uid << "still pending - waiting";
      mWaitCondition->wait( mLock );
      qDebug() << "continuing";
    }
  }

  throw ItemRetrieverException( "WTF?" );
}

// called within the retrieval thread
void ItemRetrievalManager::processRequest()
{
  qDebug() << "processRequest() - current thread:" << QThread::currentThread() << " retrieval thread:" << thread();
  mLock->lockForRead();
  // TODO: check if there is another one for the same uid with more parts requested
  Request *currentRequest = 0;
  if ( !mPendingRequests.isEmpty() )
    currentRequest = mPendingRequests.first();
  mLock->unlock();

  if ( !currentRequest ) {
    akError() << "WTF: processRequest() called but no request queued!?";
    mWaitCondition->wakeAll();
    return;
  }

  qDebug() << "processing retrieval request for item" << currentRequest->id << " parts:" << currentRequest->parts;
  // call the resource
  QString errorMsg;
  OrgFreedesktopAkonadiResourceInterface *interface = resourceInterface( currentRequest->resourceId );
  if ( interface ) {
    QDBusReply<bool> reply = interface->requestItemDelivery( currentRequest->id,
                                                             QString::fromUtf8( currentRequest->remoteId ),
                                                             QString::fromUtf8( currentRequest->mimeType ),
                                                             currentRequest->parts );
    if ( !reply.isValid() )
      errorMsg = QString::fromLatin1( "Unable to retrieve item from resource: %1" ).arg( reply.error().message() );
    else if ( reply.value() == false )
      errorMsg = QString::fromLatin1( "Resource was unable to deliver item" );
  } else {
    errorMsg = QString::fromLatin1( "Unable to contact resource" );
  }

  mLock->lockForWrite();
  currentRequest->errorMsg = errorMsg;
  currentRequest->processed = true;
  mPendingRequests.removeAll( currentRequest );
  // TODO check if (*it)->parts is a subset of currentRequest->parts
  for ( QList<Request*>::Iterator it = mPendingRequests.begin(); it != mPendingRequests.end(); ) {
    if ( (*it)->id == currentRequest->id ) {
      qDebug() << "someone else requested item" << currentRequest->id << "as well, marking as processed";
      (*it)->errorMsg = errorMsg;
      (*it)->processed = true;
      it = mPendingRequests.erase( it );
    } else {
      ++it;
    }
  }
  mWaitCondition->wakeAll();
  mLock->unlock();
}

void ItemRetrievalManager::requestCollectionSync(const Collection& collection)
{
  emit syncCollection( collection.resource().name(), collection.id() );
}

void ItemRetrievalManager::triggerCollectionSync(const QString& resource, qint64 colId)
{
  OrgFreedesktopAkonadiResourceInterface *interface = resourceInterface( resource );
  if ( interface )
    interface->synchronizeCollection( colId );
}

#include "itemretrievalmanager.moc"
