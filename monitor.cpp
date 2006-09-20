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

#include <QDBusInterface>
#include <QDBusConnection>

#include <QDebug>

using namespace PIM;

class PIM::MonitorPrivate
{
  public:
    org::kde::Akonadi::NotificationManager *nm;
    QHash<QByteArray,bool> collections;
    QSet<QByteArray> resources;
    QSet<QByteArray> items;
    QSet<QByteArray> mimetypes;

    bool isCollectionMonitored( const QByteArray &path, const QByteArray &resource )
    {
      if ( isCollectionMonitored( path ) || resources.contains( resource ) )
        return true;
      return false;
    }

    bool isItemMonitored( const QByteArray &item, const QByteArray &collection,
                          const QByteArray &mimetype, const QByteArray &resource )
    {
      if ( isCollectionMonitored( collection ) || items.contains( item ) ||
           resource.contains( resource ) || mimetypes.contains( mimetype ) )
        return true;
      return false;
    }

  private:
    bool isCollectionMonitored( const QByteArray &path )
    {
      for ( QHash<QByteArray,bool>::ConstIterator it = collections.constBegin();
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

void PIM::Monitor::monitorCollection( const QByteArray & path, bool recursive )
{
  d->collections.insert( path, recursive );
}

void PIM::Monitor::monitorItem( const DataReference & ref )
{
  d->items.insert( ref.persistanceID().toLatin1() );
}

void PIM::Monitor::monitorResource(const QByteArray & resource)
{
  d->resources.insert( resource );
}

void PIM::Monitor::monitorMimeType(const QByteArray & mimetype)
{
  d->mimetypes.insert( mimetype );
}

void PIM::Monitor::slotItemChanged( const QByteArray & uid, const QByteArray & collection,
                                    const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isItemMonitored( uid, collection, mimetype, resource ) )
    emit itemChanged( DataReference( uid, QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void PIM::Monitor::slotItemAdded( const QByteArray &uid, const QByteArray &collection,
                                  const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isItemMonitored( uid, collection, mimetype, resource ) )
    emit itemAdded( DataReference( uid, QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void PIM::Monitor::slotItemRemoved( const QByteArray & uid, const QByteArray & collection,
                                    const QByteArray &mimetype, const QByteArray &resource )
{
  if ( d->isItemMonitored( uid, collection, mimetype, resource ) )
    emit itemRemoved( DataReference( uid, QString() ) );
  if ( d->isCollectionMonitored( collection, resource ) )
    emit collectionChanged( collection );
}

void PIM::Monitor::slotCollectionChanged( const QByteArray & path, const QByteArray &resource )
{
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionChanged( path );
}

void PIM::Monitor::slotCollectionAdded( const QByteArray & path, const QByteArray &resource )
{
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionAdded( path );
}

void PIM::Monitor::slotCollectionRemoved( const QByteArray & path, const QByteArray &resource )
{
  if ( d->isCollectionMonitored( path, resource ) )
    emit collectionRemoved( path );
}

bool PIM::Monitor::connectToNotificationManager( )
{
  if ( !d->nm )
    d->nm = new org::kde::Akonadi::NotificationManager("org.kde.Akonadi", "/notifications", QDBusConnection::sessionBus(), this );
  else
    return true;

  if ( !d->nm ) {
    qWarning() << "Unable to connect to notification manager";
  } else {
    connect( d->nm, SIGNAL(itemChanged(QByteArray,QByteArray,QByteArray,QByteArray)),
             SLOT(slotItemChanged(QByteArray,QByteArray,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemAdded(QByteArray,QByteArray,QByteArray,QByteArray)),
             SLOT(slotItemAdded(QByteArray,QByteArray,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemRemoved(QByteArray,QByteArray,QByteArray,QByteArray)),
             SLOT(slotItemRemoved(QByteArray,QByteArray,QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(collectionChanged(QByteArray,QByteArray)),
             SLOT(slotCollectionChanged(QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(collectionAdded(QByteArray,QByteArray)),
             SLOT(slotCollectionAdded(QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(collectionRemoved(QByteArray,QByteArray)),
             SLOT(slotCollectionRemoved(QByteArray,QByteArray)) );
    return true;
  }
  return false;
}

#include "monitor.moc"
