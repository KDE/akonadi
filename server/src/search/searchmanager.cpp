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
#include "agentsearchmanager.h"
#include "searchmanageradaptor.h"

#include "akdebug.h"
#include "agentsearchengine.h"
#include "nepomuksearchengine.h"
#include "storage/notificationcollector.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"
#include "libs/xdgbasedirs_p.h"


#include <QDir>
#include <QPluginLoader>
#include <QDBusConnection>
#include <QTimer>

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


  NotificationCollector *collector = DataStore::self()->notificationCollector();
  connect( collector, SIGNAL(notify(Akonadi::NotificationMessageV2::List)),
           this, SLOT(scheduleSearchUpdate(Akonadi::NotificationMessageV2::List)) );

  // The timer will tick 30 seconds after last change notification. If a new notification
  // is delivered in the meantime, the timer is reset
  mSearchUpdateTimer = new QTimer( this );
  mSearchUpdateTimer->setInterval( 30 * 1000 );
  mSearchUpdateTimer->setSingleShot( true );
  connect( mSearchUpdateTimer, SIGNAL(timeout()),
           this, SLOT(searchUpdateTimeout()) );
}

SearchManager::~SearchManager()
{
  qDeleteAll( mEngines );
  sInstance = 0;
}

SearchManager *SearchManager::instance()
{
  Q_ASSERT( sInstance );
  return sInstance;
}

void SearchManager::registerInstance( const QString &id )
{
  AgentSearchManager::instance()->registerInstance( id );
}

void SearchManager::unregisterInstance( const QString &id )
{
  AgentSearchManager::instance()->unregisterInstance( id );
}

QVector<AbstractSearchPlugin *> SearchManager::searchPlugins() const
{
  return mPlugins;
}

void SearchManager::loadSearchPlugins()
{
  const QString pluginOverride = QString::fromLatin1( qgetenv( "AKONADI_OVERRIDE_SEARCHPLUGIN" ) );
  if ( !pluginOverride.isEmpty() ) {
    akDebug() << "Overriding the search plugins with: " << pluginOverride;
  }

  const QStringList dirs = XdgBaseDirs::findPluginDirs();
  Q_FOREACH ( const QString &pluginDir, dirs ) {
    QDir dir( pluginDir + QLatin1String( "/akonadi" ) );
    const QStringList desktopFiles = dir.entryList( QStringList() << QLatin1String( "*.desktop" ), QDir::Files );
    qDebug() << "SEARCH MANAGER: searching in " << pluginDir + QLatin1String( "/akonadi" ) << ":" << desktopFiles;

    Q_FOREACH ( const QString &desktopFileName, desktopFiles ) {
      QSettings desktop( pluginDir + QLatin1String( "/akonadi/" ) + desktopFileName, QSettings::IniFormat );
      desktop.beginGroup( QLatin1String( "Desktop Entry" ) );
      if ( desktop.value( QLatin1String( "Type" ) ).toString() != QLatin1String( "AkonadiSearchPlugin" ) ) {
        continue;
      }

      const QString libraryName = desktop.value( QLatin1String( "X-Akonadi-Library" ) ).toString();
      // When search plugin override is active, ignore all plugins except for the override
      if ( !pluginOverride.isEmpty() && libraryName != pluginOverride ) {
        qDebug() << desktopFileName << "skipped because of AKONADI_OVERRIDE_SEARCHPLUGIN";
        continue;
      }

      const QString pluginFile = XdgBaseDirs::findPluginFile( libraryName, QStringList() << pluginDir + QLatin1String( "/akonadi") );
      QPluginLoader loader( pluginFile );
      if  ( !loader.load() ) {
        akError() << "Failed to load search plugin" << libraryName << ":" << loader.errorString();
        continue;
      }

      AbstractSearchPlugin *plugin = qobject_cast<AbstractSearchPlugin *>( loader.instance() );
      if ( !plugin ) {
        akError() << libraryName << "is not a valid Akonadi search plugin";
        continue;
      }

      qDebug() << "SearchManager: loaded search plugin" << libraryName;
      mPlugins << plugin;
    }
  }
}

void SearchManager::scheduleSearchUpdate( const NotificationMessageV2::List &notifications )
{
  QList<Entity::Id> newChanges;

  Q_FOREACH ( const NotificationMessageV2 &msg, notifications ) {
    // we don't care about collections changes
    if ( msg.type() == NotificationMessageV2::Collections ) {
      continue;
    }

    // Only these two operations can cause item to be added or removed from a
    // persistent search (removal is handled automatically)
    if ( msg.operation() == NotificationMessageV2::Add ||
         msg.operation() == NotificationMessageV2::Modify )
    {
      newChanges << msg.uids();
    }
  }

  if ( !newChanges.isEmpty() ) {
    mPendingSearchUpdateIds << newChanges;
    mSearchUpdateTimer->start();
  }
}

void SearchManager::searchUpdateTimeout()
{
  // Get all search collections, that is subcollections of "Search", which always has ID 1
  const Collection::List collections = Collection::retrieveFiltered( Collection::parentIdFullColumnName(), 1 );
  Q_FOREACH ( const Collection &collection, collections ) {
    updateSearch( collection, DataStore::self()->notificationCollector() );
  }
}

bool SearchManager::updateSearch( const Collection &collection, NotificationCollector *collector )
{
  if ( collection.queryString().size() >= 32768 ) {
    qWarning() << "The query is at least 32768 chars long, which is the maximum size supported by the akonadi db schema. The query is therefore most likely truncated and will not be executed.";
    return false;
  }
  if ( collection.queryString().isEmpty() || collection.queryLanguage().isEmpty() ) {
    return false;
  }

  QList<qint64> queryCollections;
  if ( collection.queryCollections().isEmpty() ) {
    QueryBuilder qb( Collection::tableName() );
    qb.addColumn( Collection::idColumn() );
    // Exclude search folders
    qb.addValueCondition( Collection::parentIdColumn(), Query::NotEquals, 1 );
    if ( !qb.exec() ) {
      return false;
    }

    while ( qb.query().next() ) {
      queryCollections << qb.query().value( 0 ).toLongLong();
    }
  } else {
    Q_FOREACH ( const QString &colId, collection.queryCollections().split( QLatin1Char( ' ' ) ) ) {
      queryCollections << colId.toLongLong();
    }
  }

  QStringList queryMimeTypes;
  Q_FOREACH ( const MimeType &mt, collection.mimeTypes() ) {
    queryMimeTypes << mt.name();
  }

  // Query all plugins for search results
  QSet<qint64> newMatches;
  Q_FOREACH ( AbstractSearchPlugin *plugin, mPlugins ) {
    newMatches.unite( plugin->search( collection.queryString(), queryCollections, queryMimeTypes ) );
  }


  QSet<qint64> existingMatches, removedMatches;
  {
    QueryBuilder qb( CollectionPimItemRelation::tableName() );
    qb.addColumn( CollectionPimItemRelation::rightColumn() );
    qb.addValueCondition( CollectionPimItemRelation::leftColumn(), Query::Equals, collection.id() );
    if ( !qb.exec() ) {
      return false;
    }

    while ( qb.query().next() ) {
      const qint64 id = qb.query().value( 0 ).toLongLong();
      if ( !newMatches.contains( id ) ) {
        removedMatches << id;
      } else {
        existingMatches << id;
      }
    }
  }

  qDebug() << "Got" << newMatches.count() << "results, out of which" << existingMatches.count() << "is already in the collection";

  newMatches = newMatches - existingMatches;

  const bool existingTransaction = DataStore::self()->inTransaction();
  if ( !existingTransaction ) {
    DataStore::self()->beginTransaction();
  }

  QVariantList newMatchesVariant;
  Q_FOREACH ( qint64 id, newMatches ) {
    newMatchesVariant << id;
    Collection::addPimItem( collection.id(), id );
  }

  QVariantList removedMatchesVariant;
  Q_FOREACH ( qint64 id, removedMatches ) {
    removedMatchesVariant << id;
    Collection::removePimItem( collection.id(), id );
  }

  qDebug() << "Added" << newMatches.count() << "results, removed" << removedMatches.count();

  if ( !existingTransaction && !DataStore::self()->commitTransaction() ) {
    return false;
  }

  if ( !newMatchesVariant.isEmpty() ) {
    SelectQueryBuilder<PimItem> qb;
    qb.addValueCondition( PimItem::idFullColumnName(), Query::In, newMatchesVariant );
    if ( !qb.exec() ) {
      return false;
    }
    const QVector<PimItem> newItems = qb.result();
    collector->itemsLinked( newItems, collection );
  }

  if ( !removedMatchesVariant.isEmpty() ) {
    SelectQueryBuilder<PimItem> qb;
    qb.addValueCondition( PimItem::idFullColumnName(), Query::In, removedMatchesVariant );
    if ( !qb.exec() ) {
      return false;
    }

    const QVector<PimItem> removedItems = qb.result();
    collector->itemsUnlinked( removedItems, collection );
  }

  return true;
}
