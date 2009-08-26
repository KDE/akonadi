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

#include "collection.h"


class EventQueue;
class SessionEvent;
class FakeSession;

class FakeAkonadiServer : public QThread
{
    Q_OBJECT

public:
    FakeAkonadiServer( EventQueue *eventQueue, FakeSession *fakeSession, QObject* parent = 0 );
    ~FakeAkonadiServer();
    virtual void run();

    void setCollectionStore( Akonadi::Collection::List list );

    Akonadi::Collection getCollection( Akonadi::Entity::Id );

protected:
  QByteArray getResponse( SessionEvent *sessionEvent );

private Q_SLOTS:
    void newConnection();
    void dataAvailable();
    void followUp();
    void responseProcessed();

private:
    QStringList m_data;
    EventQueue *m_eventQueue;
    FakeSession *m_fakeSession;
    QByteArray m_lastRecievedMessage;
    QLocalServer *m_localServer;
    QMutex m_mutex;
    QLocalSocket* m_localSocket;
    QHash<Akonadi::Entity::Id, Akonadi::Collection> m_collections;
};

#endif

