/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collectionlistjob.h"
#include "collectionstatusjob.h"
#include "itemfetchjob.h"
#include "monitor.h"
#include "monitor_p.h"
#include "notificationmanagerinterface.h"
#include "session.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <QtCore/QDebug>
#include <QtCore/QTimer>

using namespace Akonadi;

class Monitor::Private
{
  public:
    Private( Monitor *parent )
      : mParent( parent ),
        fetchCollection( false ),
        fetchCollectionStatus( false )
    {
    }

    Monitor *mParent;
    org::kde::Akonadi::NotificationManager *nm;
    Collection::List collections;
    QSet<QByteArray> resources;
    QSet<int> items;
    QSet<QString> mimetypes;
    bool monitorAll;
    QList<QByteArray> sessions;
    QStringList mFetchParts;

    bool isCollectionMonitored( int collection, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( uint item, int collection,
                          const QString &mimetype, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || items.contains( item ) ||
           resources.contains( resource ) || mimetypes.contains( mimetype ) )
        return true;
      return false;
    }

    bool isSessionIgnored( const QByteArray &sessionId ) const
    {
      return sessions.contains( sessionId );
    }

    bool connectToNotificationManager();

    // private slots
    void slotItemChanged( const QByteArray&, int, const QString&, int, const QString&, const QByteArray& );
    void slotItemAdded( const QByteArray&, int, const QString&, int, const QString&, const QByteArray& );
    void slotItemRemoved( const QByteArray&, int, const QString&, int, const QString&, const QByteArray& );
    void slotCollectionAdded( const QByteArray&, int, const QString&, const QByteArray& );
    void slotCollectionChanged( const QByteArray&, int, const QString&, const QByteArray& );
    void slotCollectionRemoved( const QByteArray&, int, const QString&, const QByteArray& );
    void sessionDestroyed( QObject* );
    void slotFetchItemAddedFinished( KJob* );
    void slotFetchItemChangedFinished( KJob* );
    void slotFetchCollectionAddedFinished( KJob* );
    void slotFetchCollectionChangedFinished( KJob* );
    void slotStatusChangedFinished( KJob* );
    void slotFlushRecentlyChangedCollections();

    bool fetchCollection;
    bool fetchCollectionStatus;

  private:
    // collections that need a status update
    QSet<int> recentlyChangedCollections;

    bool isCollectionMonitored( int collection ) const
    {
      if ( collections.contains( Collection( collection ) ) )
        return true;
      if ( collections.contains( Collection::root() ) )
        return true;
      return false;
    }

    void fetchStatus( int colId )
    {
      CollectionStatusJob *job = new CollectionStatusJob( Collection( colId ), mParent );
      connect( job, SIGNAL(result(KJob*)), mParent, SLOT(slotStatusChangedFinished(KJob*)) );
    }

    void notifyCollectionStatusWatchers( int collection, const QByteArray &resource )
    {
      if ( isCollectionMonitored( collection, resource ) ) {
        if (recentlyChangedCollections.empty() )
          QTimer::singleShot( 500, mParent, SLOT(slotFlushRecentlyChangedCollections()) );
        recentlyChangedCollections.insert( collection );
      }
    }
};

bool Monitor::Private::connectToNotificationManager()
{
  if ( !nm )
    nm = new org::kde::Akonadi::NotificationManager( QLatin1String( "org.kde.Akonadi" ),
                                                     QLatin1String( "/notifications" ),
                                                     QDBusConnection::sessionBus(), mParent );
  else
    return true;

  if ( !nm ) {
    qWarning() << "Unable to connect to notification manager";
  } else {
    connect( nm, SIGNAL(itemChanged(QByteArray,int,QString,int,QString,QByteArray)),
             mParent, SLOT(slotItemChanged(QByteArray,int,QString,int,QString,QByteArray)) );
    connect( nm, SIGNAL(itemAdded(QByteArray,int,QString,int,QString,QByteArray)),
             mParent, SLOT(slotItemAdded(QByteArray,int,QString,int,QString,QByteArray)) );
    connect( nm, SIGNAL(itemRemoved(QByteArray,int,QString,int,QString,QByteArray)),
             mParent, SLOT(slotItemRemoved(QByteArray,int,QString,int,QString,QByteArray)) );
    connect( nm, SIGNAL(collectionChanged(QByteArray,int,QString,QByteArray)),
             mParent, SLOT(slotCollectionChanged(QByteArray,int,QString,QByteArray)) );
    connect( nm, SIGNAL(collectionAdded(QByteArray,int,QString,QByteArray)),
             mParent, SLOT(slotCollectionAdded(QByteArray,int,QString,QByteArray)) );
    connect( nm, SIGNAL(collectionRemoved(QByteArray,int,QString,QByteArray)),
             mParent, SLOT(slotCollectionRemoved(QByteArray,int,QString,QByteArray)) );
    return true;
  }
  return false;
}

void Monitor::Private::slotItemChanged( const QByteArray &sessionId, int uid, const QString &remoteId, int collection,
                                        const QString &mimetype, const QByteArray &resource )
{
  if ( isSessionIgnored( sessionId ) )
    return;

  if ( isItemMonitored( uid, collection, mimetype, resource ) ) {
    if ( mFetchParts.isEmpty() ) {
      Item item( DataReference( uid, remoteId ) );
      item.setMimeType( mimetype );
      emit mParent->itemChanged( item, QStringList() );
    } else {
      ItemFetchJob *job = new ItemFetchJob( DataReference( uid, remoteId ), mParent );
      foreach( QString part, mFetchParts )
        job->addFetchPart( part );
      connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotFetchItemChangedFinished( KJob* ) ) );
    }
  }

  notifyCollectionStatusWatchers( collection, resource );
}

void Monitor::Private::slotItemAdded( const QByteArray &sessionId, int uid, const QString &remoteId, int collection,
                                      const QString &mimetype, const QByteArray &resource )
{
  if ( isSessionIgnored( sessionId ) )
    return;

  if ( isItemMonitored( uid, collection, mimetype, resource ) ) {
    if ( mFetchParts.isEmpty() ) {
      Item item( DataReference( uid, remoteId ) );
      item.setMimeType( mimetype );
      Collection col( collection );
      col.setResource( QString::fromUtf8( resource ) );
      emit mParent->itemAdded( item, col );
    } else {
      ItemCollectionFetchJob *job = new ItemCollectionFetchJob( DataReference( uid, remoteId ), collection, mParent );
      foreach( QString part, mFetchParts )
        job->addFetchPart( part );
      connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotFetchItemAddedFinished( KJob* ) ) );
    }
  }

  notifyCollectionStatusWatchers( collection, resource );
}

void Monitor::Private::slotItemRemoved( const QByteArray &sessionId, int uid, const QString &remoteId, int collection,
                                        const QString &mimetype, const QByteArray &resource )
{
  if ( isSessionIgnored( sessionId ) )
    return;

  if ( isItemMonitored( uid, collection, mimetype, resource ) )
    emit mParent->itemRemoved( DataReference( uid, remoteId ) );

  notifyCollectionStatusWatchers( collection, resource );
}

void Monitor::Private::slotCollectionChanged( const QByteArray &sessionId, int collection, const QString&,
                                              const QByteArray &resource )
{
  if ( isSessionIgnored( sessionId ) )
    return;

  if ( isCollectionMonitored( collection, resource ) ) {
    if ( fetchCollection ) {
      CollectionListJob *job = new CollectionListJob( Collection( collection ), CollectionListJob::Local, mParent );
      connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotFetchCollectionChangedFinished( KJob* ) ) );
    } else {
      Collection col( collection );
      col.setResource( QString::fromUtf8( resource ) );
      emit mParent->collectionChanged( col );
    }
  }
}

void Monitor::Private::slotCollectionAdded( const QByteArray &sessionId, int collection, const QString&,
                                            const QByteArray &resource )
{
  if ( isSessionIgnored( sessionId ) )
    return;

  if ( isCollectionMonitored( collection, resource ) ) {
    if ( fetchCollection ) {
      CollectionListJob *job = new CollectionListJob( Collection( collection ), CollectionListJob::Local, mParent );
      connect( job, SIGNAL( result( KJob* ) ), mParent, SLOT( slotFetchCollectionAddedFinished( KJob* ) ) );
    } else {
      Collection col( collection );
      col.setResource( QString::fromUtf8( resource ) );
      emit mParent->collectionAdded( col );
    }
  }
}

void Monitor::Private::slotCollectionRemoved( const QByteArray &sessionId, int collection,
                                              const QString &remoteId, const QByteArray &resource )
{
  if ( isSessionIgnored( sessionId ) )
    return;

  if ( isCollectionMonitored( collection, resource ) )
    emit mParent->collectionRemoved( collection, remoteId );

  // no need for status updates anymore
  recentlyChangedCollections.remove( collection );
}

void Monitor::Private::sessionDestroyed( QObject * object )
{
  Session* session = qobject_cast<Session*>( object );
  if ( session )
    sessions.removeAll( session->sessionId() );
}

void Monitor::Private::slotFetchItemAddedFinished( KJob *job )
{
  if ( job->error() ) {
    qWarning() << "Error on fetching item: " << job->errorText();
  } else {
    ItemCollectionFetchJob *fetchJob = qobject_cast<ItemCollectionFetchJob*>( job );

    const Item item = fetchJob->item();
    const Collection collection = fetchJob->collection();
    if ( item.isValid() )
      emit mParent->itemAdded( item, collection );
  }
}

void Monitor::Private::slotFetchItemChangedFinished( KJob *job )
{
  if ( job->error() ) {
    qWarning() << "Error on fetching item: " << job->errorText();
  } else {
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );

    const Item item = fetchJob->items().first();
    if ( item.isValid() )
      // TODO: pass the parts from the item fetch job here
      emit mParent->itemChanged( item, QStringList( ItemFetchJob::PartAll ) );
  }
}

void Monitor::Private::slotFetchCollectionAddedFinished( KJob *job )
{
  if ( job->error() ) {
    qWarning() << "Error on fetching collection: " << job->errorText();
  } else {
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );

    const Collection collection = listJob->collections().first();
    if ( collection.isValid() )
      emit mParent->collectionAdded( collection );
  }
}

void Monitor::Private::slotFetchCollectionChangedFinished( KJob *job )
{
  if ( job->error() ) {
    qWarning() << "Error on fetching collection: " << job->errorText();
  } else {
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );

    const Collection collection = listJob->collections().first();
    if ( collection.isValid() )
      emit mParent->collectionChanged( collection );
  }
}

void Monitor::Private::slotStatusChangedFinished( KJob* job )
{
  if ( job->error() ) {
    qWarning() << "Error on fetching collection status: " << job->errorText();
  } else {
    CollectionStatusJob *statusJob = static_cast<CollectionStatusJob*>( job );
    emit mParent->collectionStatusChanged( statusJob->collection().id(), statusJob->status() );
  }
}

void Monitor::Private::slotFlushRecentlyChangedCollections()
{
  foreach( int collection, recentlyChangedCollections ) {
    if ( fetchCollectionStatus )
      fetchStatus( collection );
    else
      emit mParent->collectionStatusChanged( collection, CollectionStatus() );
  }
  recentlyChangedCollections.clear();
}


Monitor::Monitor( QObject *parent ) :
    QObject( parent ),
    d( new Private( this ) )
{
  d->nm = 0;
  d->monitorAll = false;
  d->connectToNotificationManager();
}

Monitor::~Monitor()
{
  delete d;
}

void Monitor::monitorCollection( const Collection &collection )
{
  d->collections << collection;
}

void Monitor::monitorItem( const DataReference & ref )
{
  d->items.insert( ref.id() );
}

void Monitor::monitorResource(const QByteArray & resource)
{
  d->resources.insert( resource );
}

void Monitor::monitorMimeType(const QString & mimetype)
{
  d->mimetypes.insert( mimetype );
}

void Akonadi::Monitor::monitorAll()
{
  d->monitorAll = true;
}

void Monitor::ignoreSession(Session * session)
{
  d->sessions << session->sessionId();
}

void Monitor::fetchCollection(bool enable)
{
  d->fetchCollection = enable;
}

void Monitor::addFetchPart( const QString &identifier )
{
  if ( !d->mFetchParts.contains( identifier ) )
    d->mFetchParts.append( identifier );
}

void Monitor::fetchCollectionStatus(bool enable)
{
  d->fetchCollectionStatus = enable;
}

#include "monitor.moc"
