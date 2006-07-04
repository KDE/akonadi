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

#include <QDebug>
#include <QTimer>

using namespace Akonadi;

NotificationManager* NotificationManager::mSelf = 0;

NotificationManager::NotificationManager()
  : QObject( 0 )
{
  new NotificationManagerAdaptor( this );

  QDBus::sessionBus().registerObject( "/", this, QDBusConnection::ExportAdaptors );
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

void NotificationManager::emitSignal( )
{
  static bool flip = false;
  if ( flip )
    emit collectionChanged( "res1" );
  else
    emit collectionRemoved( "res1" );
  flip = !flip;
  QTimer::singleShot( 10000, this, SLOT(emitSignal()) );
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

#include "notificationmanager.moc"
