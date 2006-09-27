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

using namespace PIM;

class PIM::MonitorPrivate
{
  public:
    org::kde::Akonadi::NotificationManager *nm;
    QHash<QString,bool> collections;
    QSet<QByteArray> resources;
    QSet<int> items;
    QSet<QByteArray> mimetypes;

    bool isCollectionMonitored( const QString &path, const QByteArray &resource )
    {
      if ( isCollectionMonitored( path ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( uint item, const QString &collection,
                          const QByteArray &mimetype, const QByteArray &resource )
    {
      if ( isCollectionMonitored( collection ) || items.contains( item ) ||
           resource.contains( resource ) || mimetypes.contains( mimetype ) )
        return true;
      return false;
    }

  private:
    bool isCollectionMonitored( const QString &path )
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

PIM::Monitor::Monitor( QObject *parent ) :
    QObject( parent ),
    d( new MonitorPrivate() )
{
  d->nm = 0;
  connectToNotificationManager();
}

PIM::Monitor::~Monitor()
{
  delete d;
}

void PIM::Monitor::monitorCollection( const QString& path, bool recursive )
{
  d->collections.insert( path, recursive );
}

void PIM::Monitor::monitorItem( const DataReference & ref )
{
  d->items.insert( ref.persistanceID() );
}

void PIM::Monitor::monitorResource(const QByteArray & resource)
{
  d->resources.insert( resource );
}

void PIM::Monitor::monitorMimeType(const QByteArray & mimetype)
{
  d->mimetypes.insert( mimetype );
}

void PIM::Monitor::slotItemChanged( const QByteArray & uid, const QString& collection,
                                    const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isItemMonitored( uid.toUInt(), collection, mimetype, resource ) )
    emit itemChanged( DataReference( uid.toUInt(), QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void PIM::Monitor::slotItemAdded( const QByteArray &uid, const QString &collection,
                                  const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isItemMonitored( uid.toUInt(), collection, mimetype, resource ) )
    emit itemAdded( DataReference( uid.toUInt(), QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void PIM::Monitor::slotItemRemoved( const QByteArray & uid, const QString &collection,
                                    const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isItemMonitored( uid.toUInt(), collection, mimetype, resource ) )
    emit itemRemoved( DataReference( uid.toUInt(), QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void PIM::Monitor::slotCollectionChanged( const QString& path, const QByteArray &resource )
{
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionChanged( path );
}

void PIM::Monitor::slotCollectionAdded( const QString& path, const QByteArray &resource )
{
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionAdded( path );
}

void PIM::Monitor::slotCollectionRemoved( const QString& path, const QByteArray &resource )
{
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionRemoved( path );
}

bool PIM::Monitor::connectToNotificationManager( )
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
    connect( d->nm, SIGNAL(itemChanged(QByteArray,QString,QByteArray,QByteArray)),
             SLOT(slotItemChanged(QByteArray,QString,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemAdded(QByteArray,QString,QByteArray,QByteArray)),
             SLOT(slotItemAdded(QByteArray,QString,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemRemoved(QByteArray,QString,QByteArray,QByteArray)),
             SLOT(slotItemRemoved(QByteArray,QString,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(collectionChanged(QString,QByteArray)),
             SLOT(slotCollectionChanged(QString,QByteArray)) );
    connect( d->nm, SIGNAL(collectionAdded(QString,QByteArray)),
             SLOT(slotCollectionAdded(QString,QByteArray)) );
    connect( d->nm, SIGNAL(collectionRemoved(QString,QByteArray)),
             SLOT(slotCollectionRemoved(QString,QByteArray)) );
    return true;
  }
  return false;
}

#include "monitor.moc"
