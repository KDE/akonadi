/*
   Copyright (C) 2008 Omat Holding B.V. <info@omat.nl>
   Copyright (C) 2009 Stephen Kelly <steveire@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

// Own
#include "fakeserver.h"

// Qt
#include <QDebug>

// KDE
#include <KDebug>

// For unlink
# include <unistd.h>

FakeAkonadiServer::FakeAkonadiServer( QObject* parent ) : QThread( parent )
{
     moveToThread(this);
}


FakeAkonadiServer::~FakeAkonadiServer()
{
  quit();
  wait();
}

void FakeAkonadiServer::dataAvailable()
{
    QMutexLocker locker(&m_mutex);

    QByteArray data = m_localSocket->readAll();

    if (data.startsWith("0 LOGIN"))
    {
      m_localSocket->write( "0 OK User logged in\r\n" );
      return;
    }

    Q_ASSERT( !m_data.isEmpty() );

    QByteArray toWrite = QString( m_data.takeFirst() + "\r\n" ).toLatin1();

    Q_FOREVER {
        m_localSocket->write( toWrite );
        if (toWrite.startsWith("* ")) {
          toWrite = QString( m_data.takeFirst() + "\r\n" ).toLatin1();
        } else
          break;
    }
}

void FakeAkonadiServer::newConnection()
{
    QMutexLocker locker(&m_mutex);

    m_localSocket = m_localServer->nextPendingConnection();
    m_localSocket->write( QByteArray( "* OK Akonadi Fake Server [PROTOCOL 17]\r\n" ) );
    connect(m_localSocket, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
}

void FakeAkonadiServer::run()
{
    m_localServer = new QLocalServer();
    const QString socketFile = QLatin1String( "/tmp/akonadi-test/fakeakonadiserver.socket" );
    unlink( socketFile.toUtf8().constData() );
    if ( !m_localServer->listen( socketFile ) ) {
        kFatal() << "Unable to start the server";
    }

    connect(m_localServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

    m_data += QString("Foo");
    exec();
    disconnect(m_localSocket, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
    delete m_localServer;
}

void FakeAkonadiServer::setResponse( const QStringList& data )
{
    QMutexLocker locker(&m_mutex);

    m_data+= data;
}

// #include "fakeserver.moc"
