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

#include "monitor.h"
#include "notificationmanagerinterface.h"
#include "session.h"

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::MonitorPrivate
{
  public:
    org::kde::Akonadi::NotificationManager *nm;
    Collection::List collections;
    QSet<QByteArray> resources;
    QSet<int> items;
    QSet<QByteArray> mimetypes;
    bool monitorAll;
    QList<QByteArray> sessions;

    bool isCollectionMonitored( int collection, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( uint item, int collection,
                          const QByteArray &mimetype, const QByteArray &resource ) const
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

  private:
    bool isCollectionMonitored( int collection ) const
    {
      if ( collections.contains( Collection( collection ) ) )
        return true;
      if ( collections.contains( Collection::root() ) )
        return true;
      return false;
    }
};

Monitor::Monitor( QObject *parent ) :
    QObject( parent ),
    d( new MonitorPrivate() )
{
  d->nm = 0;
  d->monitorAll = false;
  connectToNotificationManager();
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
  d->items.insert( ref.persistanceID() );
}

void Monitor::monitorResource(const QByteArray & resource)
{
  d->resources.insert( resource );
}

void Monitor::monitorMimeType(const QByteArray & mimetype)
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

void Monitor::slotItemChanged( const QByteArray &sessionId, int uid, const QString &remoteId, int collection,
                               const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isItemMonitored( uid, collection, mimetype, resource ) )
    emit itemChanged( DataReference( uid, remoteId ) );
  // FIXME: only collection status has changed here
//   if ( d->isCollectionMonitored( collection, resource ) )
//     emit collectionChanged( collection );
}

void Monitor::slotItemAdded( const QByteArray &sessionId, int uid, const QString &remoteId, int collection,
                             const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isItemMonitored( uid, collection, mimetype, resource ) )
    emit itemAdded( DataReference( uid, remoteId ) );
  // FIXME: only collection status has changed here
//   if ( d->isCollectionMonitored( collection, resource ) )
//     emit collectionChanged( collection );
}

void Monitor::slotItemRemoved( const QByteArray &sessionId, int uid, const QString &remoteId, int collection,
                               const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isItemMonitored( uid, collection, mimetype, resource ) )
    emit itemRemoved( DataReference( uid, remoteId ) );
  // FIXME: only collection status has changed here
//   if ( d->isCollectionMonitored( collection, resource ) )
//     emit collectionChanged( collection );
}

void Monitor::slotCollectionChanged( const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection, remoteId );
}

void Monitor::slotCollectionAdded( const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionAdded( collection, remoteId );
}

void Monitor::slotCollectionRemoved( const QByteArray &sessionId, int collection, const QString &remoteId, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionRemoved( collection, remoteId );
}

bool Monitor::connectToNotificationManager( )
{
  if ( !d->nm )
    d->nm = new org::kde::Akonadi::NotificationManager( QLatin1String( "org.kde.Akonadi" ),
                                                        QLatin1String( "/notifications" ),
                                                        QDBusConnection::sessionBus(), this );
  else
    return true;

  if ( !d->nm ) {
    qWarning() << "Unable to connect to notification manager";
  } else {
    connect( d->nm, SIGNAL(itemChanged(QByteArray,int,QString,int,QByteArray,QByteArray)),
             SLOT(slotItemChanged(QByteArray,int,QString,int,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemAdded(QByteArray,int,QString,int,QByteArray,QByteArray)),
             SLOT(slotItemAdded(QByteArray,int,QString,int,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemRemoved(QByteArray,int,QString,int,QByteArray,QByteArray)),
             SLOT(slotItemRemoved(QByteArray,int,QString,int,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(collectionChanged(QByteArray,int,QString,QByteArray)),
             SLOT(slotCollectionChanged(QByteArray,int,QString,QByteArray)) );
    connect( d->nm, SIGNAL(collectionAdded(QByteArray,int,QString,QByteArray)),
             SLOT(slotCollectionAdded(QByteArray,int,QString,QByteArray)) );
    connect( d->nm, SIGNAL(collectionRemoved(QByteArray,int,QString,QByteArray)),
             SLOT(slotCollectionRemoved(QByteArray,int,QString,QByteArray)) );
    return true;
  }
  return false;
}

void Monitor::sessionDestroyed(QObject * obj)
{
  Session* session = dynamic_cast<Session*>( obj );
  if ( session )
    d->sessions.removeAll( session->sessionId() );
}

#include "monitor.moc"
