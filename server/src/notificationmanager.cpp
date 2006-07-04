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

#include <QtCore/QDebug>
#include <QtCore/QTimer>

#include "notificationmanager.h"
#include "notificationmanageradaptor.h"
#include "tracer.h"

using namespace Akonadi;

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  new NotificationManagerAdaptor( this );

  QDBus::sessionBus().registerObject( "/notifications", this, QDBusConnection::ExportAdaptors );

  QTimer *timer = new QTimer( this );
  connect( timer, SIGNAL( timeout() ), this, SLOT( dummy() ) );
  timer->start( 5000 );
}

NotificationManager::~NotificationManager()
{
}

NotificationManager* NotificationManager::self()
{
  if ( !mSelf )
    mSelf = new NotificationManager();

  return mSelf;
}

void NotificationManager::monitorCollection( const QByteArray & id )
{
  mMutex.lock();

  if ( !mIds.contains( id ) )
    mIds.insert( id, 0 );
  else
    mIds[ id ]++;

  mMutex.unlock();

  qDebug() << "Got monitor request for collection: " << id;
}

void NotificationManager::monitorItem( const QByteArray & id )
{
  mMutex.lock();

  if ( !mIds.contains( id ) )
    mIds.insert( id, 0 );
  else
    mIds[ id ]++;

  mMutex.unlock();

  qDebug() << "Got monitor request for item: " << id;
}

void NotificationManager::dummy()
{
  static int i = 0, j = 0;

  if ( i ) {
    i = 0;
    Tracer::self()->signalEmitted( "itemChanged", QString( "%1 %2" ).arg( QString::number( j ), "foobar" ) );
    emit itemChanged( QByteArray::number( j ), "foobar" );
  } else {
    i = 1;
    Tracer::self()->signalEmitted( "collectionChanged", QString::number( j ) );
    emit collectionChanged( QByteArray::number( j ) );
  }

  j++;
}

#include "notificationmanager.moc"
