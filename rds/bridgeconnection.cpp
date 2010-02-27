/***************************************************************************
 *   Copyright (C) 2010 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "bridgeconnection.h"

#include <libs/xdgbasedirs_p.h>

#include <QMetaObject>
#include <QTcpSocket>
#include <QLocalSocket>
#include <QSettings>

BridgeConnection::BridgeConnection( QTcpSocket* remoteSocket, QObject *parent) :
    QObject(parent),
    m_localSocket( new QLocalSocket( this ) ),
    m_remoteSocket( remoteSocket )
{
  connect( m_localSocket, SIGNAL(disconnected()), SLOT(deleteLater()) );
  connect( m_remoteSocket, SIGNAL(disconnected()), SLOT(deleteLater()) );
  connect( m_localSocket, SIGNAL(readyRead()), SLOT(slotDataAvailable()) );
  connect( m_remoteSocket, SIGNAL(readyRead()), SLOT(slotDataAvailable()) );

  // wait for the vtable to be complete
  QMetaObject::invokeMethod( this, "connectLocal", Qt::QueuedConnection );
}

BridgeConnection::~BridgeConnection()
{
  delete m_remoteSocket;
}

void BridgeConnection::slotDataAvailable()
{
  if ( m_localSocket->bytesAvailable() > 0 )
    m_remoteSocket->write( m_localSocket->read( m_localSocket->bytesAvailable() ) );
  if ( m_remoteSocket->bytesAvailable() > 0 )
    m_localSocket->write( m_remoteSocket->read( m_remoteSocket->bytesAvailable() ) );
}

void AkonadiBridgeConnection::connectLocal()
{
  const QSettings connectionSettings( Akonadi::XdgBaseDirs::akonadiConnectionConfigFile(), QSettings::IniFormat );
#ifdef Q_OS_WIN  //krazy:exclude=cpp
    const QString namedPipe = connectionSettings.value( QLatin1String( "Data/NamedPipe" ), QLatin1String( "Akonadi" ) ).toString();
    m_localSocket->connectToServer( namedPipe );
#else
    const QString defaultSocketDir = Akonadi::XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
    const QString path = connectionSettings.value( QLatin1String( "Data/UnixPath" ), defaultSocketDir + QLatin1String( "/akonadiserver.socket" ) ).toString();
    m_localSocket->connectToServer( path );
#endif
}
