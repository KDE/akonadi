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
#include "libs/xdgbasedirs_p.h"

#include <QDir>
#include <QPluginLoader>
#include <QDBusConnection>

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
