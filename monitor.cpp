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
    QList<QByteArray> collections;

    bool isCollectionMonitored( const QByteArray &path )
    {
      foreach ( const QByteArray ba, collections ) {
        if ( path.startsWith( ba ) )
          return true;
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

void PIM::Monitor::monitorCollection( const QByteArray & path )
{
  d->collections.append( path );
  if ( connectToNotificationManager() )
    d->nm->monitorCollection( path );
}

void PIM::Monitor::monitorItem( const DataReference & ref )
{
  if ( connectToNotificationManager() )
    d->nm->monitorItem( ref.persistanceID().toLatin1() );
}

void PIM::Monitor::slotItemChanged( const QByteArray & uid, const QByteArray & collection )
{
  if ( d->isCollectionMonitored( collection ) )
    emit itemChanged( DataReference( uid, QString() ) );
}

void PIM::Monitor::slotItemAdded( const QByteArray & uid, const QByteArray & collection )
{
  if ( d->isCollectionMonitored( collection ) )
    emit itemAdded( DataReference( uid, QString() ) );
}

void PIM::Monitor::slotItemRemoved( const QByteArray & uid, const QByteArray & collection )
{
  if ( d->isCollectionMonitored( collection ) )
    emit itemRemoved( DataReference( uid, QString() ) );
}

void PIM::Monitor::slotCollectionChanged( const QByteArray & path )
{
  if ( d->isCollectionMonitored( path ) )
    emit collectionChanged( path );
}

void PIM::Monitor::slotCollectionAdded( const QByteArray & path )
{
  if ( d->isCollectionMonitored( path ) )
    emit collectionAdded( path );
}

void PIM::Monitor::slotCollectionRemoved( const QByteArray & path )
{
  if ( d->isCollectionMonitored( path ) )
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
    connect( d->nm, SIGNAL(itemChanged(QByteArray,QByteArray)), SLOT(slotItemChanged(QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemAdded(QByteArray,QByteArray)), SLOT(slotItemAdded(QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(itemRemoved(QByteArray,QByteArray)), SLOT(slotItemRemoved(QByteArray,QByteArray)) );
    connect( d->nm, SIGNAL(collectionChanged(QByteArray)), SLOT(slotCollectionChanged(QByteArray)) );
    connect( d->nm, SIGNAL(collectionAdded(QByteArray)), SLOT(slotCollectionAdded(QByteArray)) );
    connect( d->nm, SIGNAL(collectionRemoved(QByteArray)), SLOT(slotCollectionRemoved(QByteArray)) );
    return true;
  }
  return false;
}

#include "monitor.moc"
