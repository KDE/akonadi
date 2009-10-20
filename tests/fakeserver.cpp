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

#include "akonadieventqueue.h"
#include "fakesession.h"
#include <entitydisplayattribute.h>
#include <kstandarddirs.h>
#include <QDir>

FakeAkonadiServer::FakeAkonadiServer( EventQueue *eventQueue, FakeSession *fakeSession, QObject* parent )
  : QThread( parent ), m_eventQueue( eventQueue ), m_fakeSession(fakeSession)
{
  moveToThread(this);

  connect (fakeSession, SIGNAL(responseProcessed()), SLOT(responseProcessed()));
}

FakeAkonadiServer::~FakeAkonadiServer()
{
  quit();
  wait();
}


void FakeAkonadiServer::setCollectionStore(Collection::List list)
{
  foreach ( const Collection &col, list )
  {
    m_collections.insert( col.id(), col );
  }
}


Collection FakeAkonadiServer::getCollection(Entity::Id id )
{
  return m_collections.value( id );
}


QByteArray FakeAkonadiServer::getResponse( SessionEvent* sessionEvent )
{
  if (!sessionEvent->response().isEmpty())
    return sessionEvent->response();

  QByteArray response;
  QList<Entity::Id> collectionIds = sessionEvent->affectedCollections();
  if (collectionIds.isEmpty())
    // TODO: Try items.
    return QByteArray();

  Collection::List collections;

  foreach (Entity::Id id, collectionIds)
  {
    Collection col = m_collections.value( id );
    QByteArray mimeTypes;

    EntityDisplayAttribute *eda = col.attribute<EntityDisplayAttribute>();

    response += "* ";
    response += QString::number(col.id()).toLocal8Bit();
    response += " ";
    response += QString::number(col.parentCollection().id()).toLocal8Bit();
    response += " (NAME \"";
    response += col.name().toLocal8Bit();
    response += "\" MIMETYPE (";
    response += mimeTypes;
    response += ") REMOTEID \"";
    response += col.remoteId().toLocal8Bit();
    response += "\" MESSAGES 0 UNSEEN 0 SIZE 0 CACHEPOLICY (INHERIT true INTERVAL -1 CACHETIMEOUT -1 SYNCONDEMAND false LOCALPARTS (ALL)) ENTITYDISPLAY \"(\\\"";
    response += eda->displayName().toLocal8Bit();
    response += "\\\" \\\"";
    response += eda->iconName().toLocal8Bit();
    response += "\\\" )\")\r\n";
  }

  return response;
}

void FakeAkonadiServer::responseProcessed()
{
  AkonadiEvent *event = m_eventQueue->dequeue();

  SessionEvent *sessionEvent = qobject_cast<SessionEvent *>( event );
  Q_ASSERT( sessionEvent );
  Q_UNUSED( sessionEvent );
}

void FakeAkonadiServer::followUp()
{
  disconnect(m_eventQueue, SIGNAL(dequeued()), this, SLOT(followUp()));

  AkonadiEvent *nextEvent = m_eventQueue->head();
  if (!nextEvent->isSessionEvent())
  {
    connect(m_eventQueue, SIGNAL(dequeued()), this, SLOT(followUp()));
    return;
  }

  SessionEvent *sessionEvent = qobject_cast<SessionEvent *>( nextEvent );

  Q_ASSERT( sessionEvent );

  QByteArray response = getResponse( sessionEvent );

  m_localSocket->write( response );

  m_eventQueue->dequeue();

  if ( sessionEvent->hasFollowUpResponse() )
    followUp();
}

void FakeAkonadiServer::dataAvailable()
{
  QMutexLocker locker(&m_mutex);

  disconnect(m_eventQueue, SIGNAL(dequeued()), this, SLOT(dataAvailable()));
  AkonadiEvent *nextEvent = m_eventQueue->head();
  if (!nextEvent->isSessionEvent())
  {
    connect(m_eventQueue, SIGNAL(dequeued()), this, SLOT(dataAvailable()));
    return;
  }

  QByteArray data = m_localSocket->readAll();

  SessionEvent *sessionEvent = qobject_cast<SessionEvent *>( nextEvent );

  Q_ASSERT( sessionEvent );

  QByteArray response = getResponse( sessionEvent );

  Q_ASSERT( sessionEvent->trigger() == data );

  m_localSocket->write( response );

  // Broken: This should only happen in eventProcessed when FakeSession gets a signal for that.
  m_eventQueue->dequeue();

  if ( sessionEvent->hasFollowUpResponse() )
    followUp();
}

void FakeAkonadiServer::newConnection()
{
    QMutexLocker locker(&m_mutex);

    m_localSocket = m_localServer->nextPendingConnection();
    m_localSocket->write( QByteArray( "* OK Akonadi Fake Server [PROTOCOL 23]\r\n" ) );
    connect(m_localSocket, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
}

void FakeAkonadiServer::run()
{
    m_localServer = new QLocalServer();

    KStandardDirs stdirs;
    QDir homeDir(stdirs.localkdedir());

    homeDir.mkpath("xdg/local/akonadi");

    Q_ASSERT( homeDir.cd( "xdg/local/akonadi" ) );
    const QString socketFile = homeDir.absoluteFilePath("akonadiserver.socket");

    unlink( socketFile.toUtf8().constData() );
    if ( !m_localServer->listen( socketFile ) ) {
        kFatal() << "Unable to start the server" << m_localServer->serverName() << m_localServer->errorString() << m_localServer->serverError();
    }

    connect(m_localServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

    exec();
    disconnect(m_localSocket, SIGNAL(readyRead()), this, SLOT(dataAvailable()));
    delete m_localServer;
}

// #include "fakeserver.moc"
