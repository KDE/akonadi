/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>
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

#include "searchmanager.h"
#include "abstractsearchplugin.h"
#include "searchmanageradaptor.h"
#include "searchinstance.h"
#include "searchresultsretriever.h"
#include "searchresultsretrievaljob.h"

#include <akdebug.h>
#include "agentsearchengine.h"
#include "nepomuksearchengine.h"
#include "entities.h"
#include <storage/notificationcollector.h>
#include <akonadiconnection.h>
#include <libs/xdgbasedirs_p.h>

#include <QDir>
#include <QPluginLoader>
#include <QReadWriteLock>
#include <QDBusConnection>

#include <boost/scoped_ptr.hpp>

using namespace Akonadi;

SearchManager *SearchManager::sInstance = 0;

Q_DECLARE_METATYPE( Collection )
Q_DECLARE_METATYPE( QSet<qint64> )

SearchManager::SearchManager( const QStringList &searchEngines, QObject *parent )
  : QObject( parent )
  , mDBusConnection( QDBusConnection::connectToBus(
          QDBusConnection::SessionBus,
          QString::fromLatin1( "AkonadiServerSearchManager" ) ) )
{
  qRegisterMetaType< QSet<qint64> >();

  Q_ASSERT( QThread::currentThread() == QCoreApplication::instance()->thread() );
  Q_ASSERT( sInstance == 0 );
  sInstance = this;

  mLock = new QReadWriteLock();
  mInstancesLock = new QMutex();
  mWaitCondition = new QWaitCondition();

  connect( this, SIGNAL(requestAdded()), SLOT(processRequest()), Qt::QueuedConnection );

  qRegisterMetaType<Collection>();

  mEngines.reserve( searchEngines.size() );
  Q_FOREACH ( const QString &engineName, searchEngines ) {
    if ( engineName == QLatin1String( "Nepomuk" ) ) {
#ifdef HAVE_SOPRANO
      m_engines.append( new NepomukSearchEngine );
#else
      akError() << "Akonadi has been built without Nepomuk support!";
#endif
    } else if ( engineName == QLatin1String( "Agent" ) ) {
      mEngines.append( new AgentSearchEngine );
    } else {
      akError() << "Unknown search engine type: " << engineName;
    }
  }

  loadSearchPlugins();

  new SearchManagerAdaptor( this );

  QDBusConnection::sessionBus().registerObject(
      QLatin1String( "/SearchManager" ),
      this,
      QDBusConnection::ExportAdaptors );
}

void SearchManager::loadSearchPlugins()
{
  const QStringList dirs = Akonadi::XdgBaseDirs::systemPathList( "LD_LIBRARY_PATH" );
  #if defined(Q_OS_WIN) //krazy:exclude=cpp
    const QString filter = QLatin1String( "*.dll" );
  #else
    const QString filter = QLatin1String( "*.so" );
  #endif
  Q_FOREACH ( const QString &dir, dirs ) {
    const QDir directory( dir + QDir::separator() + QLatin1String( "akonadi/plugins" ), filter );
    const QStringList files = directory.entryList();
    for ( int i = 0; i < files.count(); ++i ) {
      const QString filePath = directory.absoluteFilePath( files[i] );

      QPluginLoader loader( filePath );
      if ( !loader.load() ) {
        qWarning() << "Failed to load search plugin " << files[i] << ":" << loader.errorString();
        continue;
      }

      AbstractSearchPlugin *plugin = reinterpret_cast<AbstractSearchPlugin*>( loader.instance() );
      if ( !plugin ) {
        loader.unload();
        qWarning() << "Failed to obtain instance of" << files[i] << "search plugin:" << loader.errorString();
      }

      mPlugins << plugin;
      akDebug() << "SEARCH PLUGIN:" << files[i];
    }
  }
}

SearchManager::~SearchManager()
{
  qDeleteAll( mEngines );
  qDeleteAll( mPlugins );
  qDeleteAll( mSearchInstances );
  sInstance = 0;
  delete mWaitCondition;
  delete mLock;
  delete mInstancesLock;
}

SearchManager *SearchManager::instance()
{
  Q_ASSERT( sInstance );
  return sInstance;
}

void SearchManager::registerInstance( const QString &id )
{
  QMutexLocker locker( mInstancesLock );

  akDebug() << "SearchManager::registerInstance(" << id << ")";

  SearchInstance *instance = mSearchInstances.value( id );
  if ( instance ) {
    return; // already registered
  }

  instance = new SearchInstance( id );
  if ( !instance->init() ) {
    akDebug() << "Failed to initialize Search agent";
    delete instance;
    return;
  }

  akDebug() << "Registering search instance " << id;

  mSearchInstances.insert( id, instance );
}

void SearchManager::unregisterInstance( const QString &id )
{
  QMutexLocker locker( mInstancesLock );

  SearchInstance *instance = mSearchInstances.value( id );
  if ( instance ) {
    akDebug() << "Unregistering search instance" << id;
    delete mSearchInstances.take( id );
  }
}


bool SearchManager::addSearch( const Collection &collection )
{
  if ( collection.queryString().size() >= 32768 ) {
    qWarning() << "The query is at least 32768 chars long, which is the maximum size supported by the akonadi db schema. The query is therefore most likely truncated and will not be executed.";
    return false;
  }
  if ( collection.queryString().isEmpty() || collection.queryLanguage().isEmpty() ) {
    return false;
  }

  // send to the main thread
  QMetaObject::invokeMethod( this, "addSearchInternal", Qt::QueuedConnection, Q_ARG( Collection, collection ) );
  return true;
}

void SearchManager::addSearchInternal( const Collection &collection )
{
  Q_FOREACH ( AbstractSearchEngine *engine, mEngines ) {
    engine->addSearch( collection );
  }
}

bool SearchManager::removeSearch( qint64 id )
{
  // send to the main thread
  QMetaObject::invokeMethod( this, "removeSearchInternal", Qt::QueuedConnection, Q_ARG( qint64, id ) );
  return true;
}

void SearchManager::removeSearchInternal( qint64 id )
{
  Q_FOREACH ( AbstractSearchEngine *engine, mEngines ) {
    engine->removeSearch( id );
  }
}

void SearchManager::updateSearch( const Akonadi::Collection &collection, NotificationCollector *collector )
{
  removeSearch( collection.id() );
  collector->itemsUnlinked( collection.pimItems(), collection );
  collection.clearPimItems();
  addSearch( collection );
}

void SearchManager::processRequest()
{
  QVector<QPair<SearchResultsRetrievalJob *, QString> > newJobs;

  mLock->lockForWrite();
  // look for idle resources
  for ( QHash<QString, QList<SearchRequest *> >::Iterator it = mPendingRequests.begin(); it != mPendingRequests.end(); ) {
    if ( it.value().isEmpty() ) {
      it = mPendingRequests.erase( it );
      continue;
    }

    if ( !mCurrentJobs.contains( it.key() ) || mCurrentJobs.value( it.key() ) == 0 ) {
      SearchRequest *req = it.value().takeFirst();
      Q_ASSERT( req->resourceId == it.key() );
      SearchResultsRetrievalJob *job = new SearchResultsRetrievalJob( req, this );
      connect( job, SIGNAL(requestCompleted(SearchRequest*,QSet<qint64>)),
               this, SLOT(requestCompleted(SearchRequest*,QSet<qint64>)) );
      mCurrentJobs.insert( req->resourceId, job );
      newJobs.append( qMakePair( job, req->resourceId ) );
    }
    ++it;
  }

  bool nothingGoingOn = mPendingRequests.isEmpty() && mCurrentJobs.isEmpty() && newJobs.isEmpty();
  mLock->unlock();

  if ( nothingGoingOn ) {
    mWaitCondition->wakeAll();
    return;
  }

  for ( QVector<QPair<SearchResultsRetrievalJob *, QString> >::Iterator it = newJobs.begin(); it != newJobs.end(); ++it ) {
    akDebug() << "Starting SearchResultsRetrievalJob:" << ( *it ).second << mSearchInstances.value( ( *it ).second );
    ( *it ).first->start( mSearchInstances.value( ( *it ).second ) );
  }
}

void SearchManager::searchResultsAvailable( const QByteArray &searchId,
                                            const QSet<qint64> ids,
                                            AkonadiConnection* connection )
{
  akDebug() << "Result available for search" << searchId;
  mLock->lockForWrite();
  Q_ASSERT( mCurrentJobs.contains( connection->resourceContext().name() ) );
  SearchResultsRetrievalJob *job = mCurrentJobs[connection->resourceContext().name()];
  job->setResult( ids );
  mLock->unlock();
}

void SearchManager::requestCompleted( SearchRequest *request, const QSet<qint64> &result )
{
  mLock->lockForWrite();
  request->processed = true;
  request->result = result;

  Q_ASSERT( mCurrentJobs.contains( request->resourceId ) );
  mCurrentJobs.remove( request->resourceId );

  mWaitCondition->wakeAll();
  mLock->unlock();
  Q_EMIT requestAdded(); // trigger processRequest() again
}

QSet<qint64> SearchManager::search( SearchRequest *req )
{
  // Don't bother processing the request if the owning resource does not
  // support search
  if ( !mSearchInstances.contains( req->resourceId ) ) {
    akDebug() << "Resource" << req->resourceId << "does not implement Search interface, skipping request";
    return QSet<qint64>();
  }

  mLock->lockForWrite();
  mPendingRequests[req->resourceId].append( req );
  mLock->unlock();

  Q_EMIT requestAdded();

  mLock->lockForRead();
  Q_FOREVER {
    if ( req->processed ) {
      akDebug() << "Search request processed";
      boost::scoped_ptr<SearchRequest> reqDeleter( req );
      Q_ASSERT( !mPendingRequests[req->resourceId].contains( req ) );
      const QString errorMsg = req->errorMsg;
      mLock->unlock();
      if ( errorMsg.isEmpty() ) {
        return req->result;
      } else {
        throw SearchResultsRetrieverException( errorMsg );
      }
    } else {
      akDebug() << "Waiting for search results from resource";
      mWaitCondition->wait( mLock );
    }
  }

  throw SearchResultsRetrieverException( "WTF?" );
  return QSet<qint64>();
}
