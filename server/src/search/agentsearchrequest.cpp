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

#include <QPluginLoader>

#include "agentsearchmanager.h"
#include "abstractsearchplugin.h"
#include "searchmanager.h"
#include "akonadiconnection.h"
#include "akdebug.h"
#include "xdgbasedirs_p.h"

using namespace Akonadi;

AgentSearchRequest::AgentSearchRequest( const QByteArray &connectionId )
  : mConnectionId( connectionId )
  , mRemoteSearch( true )
  , mEmitResults( true )
{
}

AgentSearchRequest::~AgentSearchRequest()
{
}

QByteArray AgentSearchRequest::connectionId() const
{
  return mConnectionId;
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

void AgentSearchRequest::setRemoteSearch( bool remote )
{
  mRemoteSearch = remote;
}

bool AgentSearchRequest::remoteSearch() const
{
  return mRemoteSearch;
}

void AgentSearchRequest::setEmitResults( bool emitResults )
{
  mEmitResults = emitResults;
}

QSet<qint64> AgentSearchRequest::results() const
{
  return mResults;
}

void AgentSearchRequest::emitResults( const QSet<qint64> &results )
{
  if ( mEmitResults ) {
    Q_EMIT resultsAvailable( results );
  } else {
    mResults.unite( results );
  }
}

void AgentSearchRequest::searchPlugins()
{
  const QVector<AbstractSearchPlugin *> plugins = SearchManager::instance()->searchPlugins();
  Q_FOREACH ( AbstractSearchPlugin *plugin, plugins ) {
    const QSet<qint64> result = plugin->search( mQuery, mCollections.toList(), mMimeTypes );
    emitResults( result );
  }
}

void AgentSearchRequest::exec()
{
  akDebug() << "Executing search" << mConnectionId;

  //TODO should we move this to the AgentSearchManager as well? If we keep it here the agents can be searched in parallel
  //since the plugin search is executed in this thread directly.
  searchPlugins();

  // If remote search is disabled, just finish here after searching the plugins
  if ( !mRemoteSearch ) {
    akDebug() << "Search done" << mConnectionId << "(without remote search)";
    return;
  }

  AgentSearchTask task;
  task.id = mConnectionId;
  task.query = mQuery;
  task.mimeTypes = mMimeTypes;
  task.collections = mCollections;
  task.complete = false;

  AgentSearchManager::instance()->addTask( &task );

  task.sharedLock.lock();
  Q_FOREVER {
    if ( task.complete ) {
      akDebug() << "All queries processed!";
      break;
    } else {
      task.notifier.wait( &task.sharedLock );

      akDebug() << task.pendingResults.count() << "search results available in search" << task.id;
      if ( !task.pendingResults.isEmpty() ) {
        emitResults( task.pendingResults );
      }
      task.pendingResults.clear();
    }
  }

  if ( !task.pendingResults.isEmpty() ) {
    if ( mEmitResults ) {
      emitResults( task.pendingResults );
    }
  }
  task.sharedLock.unlock();

  akDebug() << "Search done" << mConnectionId;
}
