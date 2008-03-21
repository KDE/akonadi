/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "akonadistarter.h"

#include <akonadi/private/protocol_p.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDebug>
#include <QProcess>
#include <QTimer>

AkonadiStarter::AkonadiStarter(QObject * parent) :
    QObject( parent ),
    mRegistered( false )
{
  connect( QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
           SLOT( serviceOwnerChanged(QString,QString,QString)) );
}

bool AkonadiStarter::start()
{
  qDebug( "Starting Akonadi Server..." );
  const bool ok = QProcess::startDetached( "akonadi_control" );
  if ( !ok ) {
    qDebug( "Error: unable to execute binary akonadi_control" );
    return false;
  }

  // safety timeout
  QTimer::singleShot( 5000, QCoreApplication::instance(), SLOT(quit()) );
  // wait for the server to register with D-Bus
  QCoreApplication::instance()->exec();

  if ( !mRegistered ) {
    qDebug( "Error: akonadi_control was started but didn't register at D-Bus session bus." );
    qDebug( "Make sure your system is setup correctly!" );
    return false;
  }

  qDebug( "   done." );
  return true;
}

void AkonadiStarter::serviceOwnerChanged(const QString & name, const QString & oldOwner, const QString & newOwner)
{
  Q_UNUSED( oldOwner );
  if ( name != QLatin1String( AKONADI_DBUS_CONTROL_SERVICE ) || newOwner.isEmpty() )
    return;
  mRegistered = true;
  QCoreApplication::instance()->quit();
}

#include "akonadistarter.moc"
