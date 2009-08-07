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

#ifndef FAKE_AKONADI_SERVER_H
#define FAKE_AKONADI_SERVER_H

// Renamed and refactored from kdepimlibs/kimap/tests/fakeserver

#include <QStringList>
#include <QLocalSocket>
#include <QThread>
#include <QMutex>
#include <QLocalServer>
#include <QLocalSocket>

class FakeAkonadiServer : public QThread
{
    Q_OBJECT

public:
    FakeAkonadiServer( QObject* parent = 0 );
    ~FakeAkonadiServer();
    virtual void run();
    void setResponse( const QStringList& data );

private Q_SLOTS:
    void newConnection();
    void dataAvailable();

private:
    QStringList m_data;
    QLocalServer *m_localServer;
    QMutex m_mutex;
    QLocalSocket* m_localSocket;
};

#endif

