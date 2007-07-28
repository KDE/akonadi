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
#include "notificationmessage.h"
#include "session.h"

#include <kdebug.h>

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
    QHash<KJob*,NotificationMessage> pendingJobs;

    bool isCollectionMonitored( int collection, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( uint item, int collection, int collectionDest,
                          const QString &mimetype, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) ||
           isCollectionMonitored( collectionDest ) ||items.contains( item ) ||
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
    void sessionDestroyed( QObject* );
    void slotStatusChangedFinished( KJob* );
    void slotFlushRecentlyChangedCollections();

    void slotNotify( const NotificationMessage::List &msgs );
    void slotItemJobFinished( KJob* job );
    void slotCollectionJobFinished( KJob *job );

    void emitItemNotification( const NotificationMessage &msg, const Item &item = Item(),
                               const Collection &collection = Collection(), const Collection &collectionDest = Collection() );
    void emitCollectionNotification( const NotificationMessage &msg, const Collection &col = Collection(),
                                     const Collection &par = Collection() );

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
  NotificationMessage::registerDBusTypes();

  if ( !nm )
    nm = new org::kde::Akonadi::NotificationManager( QLatin1String( "org.kde.Akonadi" ),
                                                     QLatin1String( "/notifications" ),
                                                     QDBusConnection::sessionBus(), mParent );
  else
    return true;

  if ( !nm ) {
    qWarning() << "Unable to connect to notification manager";
  } else {
    connect( nm, SIGNAL(notify(Akonadi::NotificationMessage::List)),
             mParent, SLOT(slotNotify(Akonadi::NotificationMessage::List)) );
    return true;
  }
  return false;
}

void Monitor::Private::sessionDestroyed( QObject * object )
{
  Session* session = qobject_cast<Session*>( object );
  if ( session )
    sessions.removeAll( session->sessionId() );
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
    if ( fetchCollectionStatus ) {
      fetchStatus( collection );
    } else {
      static const CollectionStatus dummyStatus;
      emit mParent->collectionStatusChanged( collection, dummyStatus );
    }
  }
  recentlyChangedCollections.clear();
}

void Monitor::Private::slotNotify( const NotificationMessage::List &msgs )
{
  foreach ( const NotificationMessage msg, msgs ) {
    if ( isSessionIgnored( msg.sessionId() ) )
      continue;

    if ( msg.type() == NotificationMessage::Item ) {
      notifyCollectionStatusWatchers( msg.parentCollection(), msg.resource() );
      if ( !isItemMonitored( msg.uid(), msg.parentCollection(), msg.parentDestCollection(), msg.mimeType(), msg.resource() ) )
        continue;
      if ( !mFetchParts.isEmpty() &&
           ( msg.operation() == NotificationMessage::Add || msg.operation() == NotificationMessage::Move )) {
        ItemCollectionFetchJob *job = new ItemCollectionFetchJob( DataReference( msg.uid(), msg.remoteId() ),
                                                                  msg.parentCollection(), mParent );
        foreach( QString part, mFetchParts )
          job->addFetchPart( part );
        pendingJobs.insert( job, msg );
        connect( job, SIGNAL(result(KJob*)), mParent, SLOT(slotItemJobFinished(KJob*)) );
        continue;
      }
      if ( !mFetchParts.isEmpty() && msg.operation() == NotificationMessage::Modify ) {
        ItemFetchJob *job = new ItemFetchJob( DataReference( msg.uid(), msg.remoteId() ), mParent );
        foreach( QString part, mFetchParts )
          job->addFetchPart( part );
        pendingJobs.insert( job, msg );
        connect( job, SIGNAL(result(KJob*)), mParent, SLOT(slotItemJobFinished(KJob*)) );
        continue;
      }
      emitItemNotification( msg );
    } else if ( msg.type() == NotificationMessage::Collection ) {
      if ( msg.operation() != NotificationMessage::Remove && fetchCollection ) {
        Collection::List list;
        list << Collection( msg.uid() );
        if ( msg.operation() == NotificationMessage::Add )
          list << Collection( msg.parentCollection() );
        CollectionListJob *job = new CollectionListJob( list, mParent );
        pendingJobs.insert( job, msg );
        connect( job, SIGNAL(result(KJob*)), mParent, SLOT(slotCollectionJobFinished(KJob*)) );
        continue;
      }
      if ( msg.operation() == NotificationMessage::Remove ) {
        // no need for status updates anymore
        recentlyChangedCollections.remove( msg.uid() );
      }
      emitCollectionNotification( msg );
    } else {
      qWarning() << "Received unknown change notification!";
    }
  }
}

void Monitor::Private::emitItemNotification( const NotificationMessage &msg, const Item &item,
                                             const Collection &collection, const Collection &collectionDest  )
{
  Q_ASSERT( msg.type() == NotificationMessage::Item );
  Collection col = collection;
  Collection colDest = collectionDest;
  if ( !col.isValid() ) {
    col = Collection( msg.parentCollection() );
    col.setResource( QString::fromUtf8( msg.resource() ) );
  }
  if ( !colDest.isValid() ) {
    colDest = Collection( msg.parentDestCollection() );
    // FIXME setResource here required ?
  }
  Item it = item;
  if ( !it.isValid() ) {
    it = Item( DataReference( msg.uid(), msg.remoteId() ) );
    it.setMimeType( msg.mimeType() );
  }
  switch ( msg.operation() ) {
    case NotificationMessage::Add:
      emit mParent->itemAdded( it, col );
      break;
    case NotificationMessage::Modify:
      emit mParent->itemChanged( it, msg.itemParts() );
      break;
    case NotificationMessage::Move:
      emit mParent->itemMoved( it, col, colDest );
      break;
    case NotificationMessage::Remove:
      emit mParent->itemRemoved( it.reference() );
      break;
    default:
      Q_ASSERT_X( false, "Monitor::Private::emitItemNotification()", "Invalid enum value" );
  }
}

void Monitor::Private::emitCollectionNotification( const NotificationMessage &msg, const Collection &col,
                                                   const Collection &par )
{
  Q_ASSERT( msg.type() == NotificationMessage::Collection );
  Collection collection = col;
  if ( !collection.isValid() ) {
    collection = Collection( msg.uid() );
    collection.setParent( msg.parentCollection() );
    collection.setResource( QString::fromUtf8( msg.resource() ) );
    collection.setRemoteId( msg.remoteId() );
  }
  Collection parent = par;
  if ( !parent.isValid() )
    parent = Collection( msg.parentCollection() );
  switch ( msg.operation() ) {
    case NotificationMessage::Add:
      emit mParent->collectionAdded( collection, parent );
      break;
    case NotificationMessage::Modify:
      emit mParent->collectionChanged( collection );
      break;
    case NotificationMessage::Remove:
      emit mParent->collectionRemoved( collection.id(), collection.remoteId() );
      break;
    default:
      Q_ASSERT_X( false, "Monitor::Private::emitCollectionNotification", "Invalid enum value" );
  }
}

void Monitor::Private::slotItemJobFinished( KJob* job )
{
  if ( !pendingJobs.contains( job ) ) {
    kWarning() << k_funcinfo << "unknown job - wtf is going on here?" << endl;
    return;
  }
  NotificationMessage msg = pendingJobs.take( job );
  if ( job->error() ) {
    kWarning() << k_funcinfo << "Error on fetching item: " << job->errorText() << endl;
  } else {
    Item item;
    Collection col;
    ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
    if ( fetchJob && fetchJob->items().count() > 0 )
      item = fetchJob->items().first();
    ItemCollectionFetchJob *cfjob = qobject_cast<ItemCollectionFetchJob*>( job );
    if ( cfjob ) {
      item = cfjob->item();
      col = cfjob->collection();
    }
    emitItemNotification( msg, item, col );
  }
}

void Monitor::Private::slotCollectionJobFinished( KJob* job )
{
  if ( !pendingJobs.contains( job ) ) {
    kWarning() << k_funcinfo << "unknown job - wtf is going on here?" << endl;
    return;
  }
  NotificationMessage msg = pendingJobs.take( job );
  if ( job->error() ) {
    kWarning() << k_funcinfo << "Error on fetching collection: " << job->errorText() << endl;
  } else {
    Collection col, parent;
    CollectionListJob *listJob = qobject_cast<CollectionListJob*>( job );
    if ( listJob && listJob->collections().count() > 0 )
      col = listJob->collections().first();
    if ( listJob && listJob->collections().count() > 1 && msg.operation() == NotificationMessage::Add ) {
      parent = listJob->collections().at( 1 );
      if ( col.id() != msg.uid() )
        qSwap( col, parent );
    }
    emitCollectionNotification( msg, col, parent );
  }
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
