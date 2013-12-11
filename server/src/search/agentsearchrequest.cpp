/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "agentsearchrequest.h"
#include "agentsearchmanager.h"
#include "akonadiconnection.h"
#include "akdebug.h"

using namespace Akonadi;

AgentSearchRequest::AgentSearchRequest( Akonadi::AkonadiConnection *connection )
  : mConnection( connection )
{
}

AgentSearchRequest::~AgentSearchRequest()
{
}

AkonadiConnection *AgentSearchRequest::connection() const
{
  return mConnection;
}

void AgentSearchRequest::setQuery( const QString &query )
{
  mQuery = query;
}

QString AgentSearchRequest::query() const
{
  return mQuery;
}

void AgentSearchRequest::setCollections( const QVector<qint64> &collectionsIds )
{
  mCollections = collectionsIds;
}

QVector<qint64> AgentSearchRequest::collections() const
{
  return mCollections;
}


void AgentSearchRequest::setMimeTypes( const QStringList &mimeTypes )
{
  mMimeTypes = mimeTypes;
}

QStringList AgentSearchRequest::mimeTypes() const
{
  return mMimeTypes;
}

void AgentSearchRequest::exec()
{
  akDebug() << "Executing search" << mConnection->sessionId();

  AgentSearchTask task;
  task.id = mConnection->sessionId();
  task.query = mQuery;
  task.mimeTypes = mMimeTypes;
  task.collections = mCollections;

  AgentSearchManager::instance()->addTask( &task );

  task.sharedLock.lock();
  Q_FOREVER {
    if ( task.queries.isEmpty() ) {
      akDebug() << "All queries processed!";
      break;
    } else {
      task.notifier.wait( &task.sharedLock );

      akDebug() << task.pendingResults.count() << "search results available in search" << task.id;
      if ( !task.pendingResults.isEmpty() ) {
        Q_EMIT resultsAvailable( task.pendingResults );
      }
      task.pendingResults.clear();
    }
  }

  if ( !task.pendingResults.isEmpty() ) {
    Q_EMIT resultsAvailable( task.pendingResults );
  }
  task.sharedLock.unlock();

  akDebug() << "Search done" << mConnection->sessionId();
}
