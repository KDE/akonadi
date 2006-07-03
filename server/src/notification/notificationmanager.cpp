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

#include "notificationmanager.h"
#include "notificationmanageradaptor.h"
#include "notificationmanagerinterface.h"

#include <QDebug>
#include <QTimer>

using namespace Akonadi;

Akonadi::NotificationManager::NotificationManager( ) : QObject( 0 )
{
  new NotificationManagerAdaptor( this );

  QDBus::sessionBus().registerObject( "/", this, QDBusConnection::ExportAdaptors );

  new org::kde::Akonadi::NotificationManager( QString(), QString(), QDBus::sessionBus(), this );
}

void Akonadi::NotificationManager::emitSignal( )
{
  static bool flip = false;
  if ( flip )
    emit collectionChanged( "res1" );
  else
    emit collectionRemoved( "res1" );
  flip = !flip;
  QTimer::singleShot( 10000, this, SLOT(emitSignal()) );
}

void Akonadi::NotificationManager::monitorCollection( const QByteArray & path )
{
  qDebug() << "Got monitor request for collection: " << path;
}

void Akonadi::NotificationManager::monitorItem( const QByteArray & uid )
{
  qDebug() << "Got monitor request for item: " << uid;
}

#include "notificationmanager.moc"
