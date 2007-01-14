/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusConnection>

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::MonitorPrivate
{
  public:
    org::kde::Akonadi::NotificationManager *nm;
    QHash<QString,bool> collections;
    QSet<QByteArray> resources;
    QSet<int> items;
    QSet<QByteArray> mimetypes;
    bool monitorAll;
    QList<int> sessions;

    bool isCollectionMonitored( const QString &path, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( path ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( uint item, const QString &collection,
                          const QByteArray &mimetype, const QByteArray &resource ) const
    {
      if ( monitorAll || isCollectionMonitored( collection ) || items.contains( item ) ||
           resource.contains( resource ) || mimetypes.contains( mimetype ) )
        return true;
      return false;
    }

    bool isSessionIgnored( int sessionId ) const
    {
      return sessions.contains( sessionId );
    }

  private:
    bool isCollectionMonitored( const QString &path ) const
    {
      for ( QHash<QString,bool>::ConstIterator it = collections.constBegin();
            it != collections.constEnd(); ++it ) {
        if ( it.value() ) {
          if ( path.startsWith( it.key() ) )
            return true;
        } else {
          if ( path == it.key() )
            return true;
        }
      }
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

void Monitor::monitorCollection( const QString& path, bool recursive )
{
  d->collections.insert( path, recursive );
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

void Monitor::ignoreSession(Job * session)
{
  d->sessions << session->sessionId();
}

void Monitor::slotItemChanged( int sessionId, const QByteArray & uid, const QString& collection,
                               const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isItemMonitored( uid.toUInt(), collection, mimetype, resource ) )
    emit itemChanged( DataReference( uid.toUInt(), QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void Monitor::slotItemAdded( int sessionId, const QByteArray &uid, const QString &collection,
                             const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isItemMonitored( uid.toUInt(), collection, mimetype, resource ) )
    emit itemAdded( DataReference( uid.toUInt(), QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void Monitor::slotItemRemoved( int sessionId, const QByteArray & uid, const QString &collection,
                               const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isItemMonitored( uid.toUInt(), collection, mimetype, resource ) )
    emit itemRemoved( DataReference( uid.toUInt(), QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void Monitor::slotCollectionChanged( int sessionId, const QString& path, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionChanged( path );
}

void Monitor::slotCollectionAdded( int sessionId, const QString& path, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionAdded( path );
}

void Monitor::slotCollectionRemoved( int sessionId, const QString& path, const QByteArray &resource )
{
  if ( d->isSessionIgnored( sessionId ) ) return;
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionRemoved( path );
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
    connect( d->nm, SIGNAL(itemChanged(int,QByteArray,QString,QByteArray,QByteArray)),
             SLOT(slotItemChanged(int,QByteArray,QString,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemAdded(int,QByteArray,QString,QByteArray,QByteArray)),
             SLOT(slotItemAdded(int,QByteArray,QString,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemRemoved(int,QByteArray,QString,QByteArray,QByteArray)),
             SLOT(slotItemRemoved(int,QByteArray,QString,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(collectionChanged(int,QString,QByteArray)),
             SLOT(slotCollectionChanged(int,QString,QByteArray)) );
    connect( d->nm, SIGNAL(collectionAdded(int,QString,QByteArray)),
             SLOT(slotCollectionAdded(int,QString,QByteArray)) );
    connect( d->nm, SIGNAL(collectionRemoved(int,QString,QByteArray)),
             SLOT(slotCollectionRemoved(int,QString,QByteArray)) );
    return true;
  }
  return false;
}

void Monitor::sessionDestroyed(QObject * obj)
{
  Job* job = dynamic_cast<Job*>( obj );
  if ( job )
    d->sessions.removeAll( job->sessionId() );
}

#include "monitor.moc"
