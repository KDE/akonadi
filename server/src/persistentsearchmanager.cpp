/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include <QDir>
#include <QSqlQuery>
#include <QStringList>
#include <QUuid>
#include <QVariant>

#include "handlerhelper.h"
#include "searchhelper.h"
#include "searchprovidermanager.h"

#include "persistentsearchmanager.h"

using namespace Akonadi;

PersistentSearchManager* PersistentSearchManager::mSelf = 0;

PersistentSearchManager::PersistentSearchManager()
{
  const QString akonadiHomeDir = QDir::homePath() + QDir::separator() + QLatin1String(".akonadi") + QDir::separator();
  if ( !QDir( akonadiHomeDir ).exists() ) {
    QDir dir;
    dir.mkdir( akonadiHomeDir );
  }

  mDatabase = QSqlDatabase::addDatabase( QLatin1String("QSQLITE"), QUuid::createUuid().toString() );
  mDatabase.setDatabaseName( akonadiHomeDir + QLatin1String("akonadi.db") );
  mDatabase.open();

  if ( !mDatabase.isOpen() )
    qDebug() << "PersistentSearchManager: Unable to open database akonadi.db.";

  QSqlQuery query( mDatabase );
  query.prepare( QLatin1String("SELECT name, query FROM PersistentSearches") );
  if ( query.exec() ) {
    qDebug( "load data" );
    mListMutex.lock();
    while ( query.next() ) {
      QString identifier = query.value( 0 ).toString();
    qDebug( "found %s", qPrintable( identifier ) );

      QString queryString = query.value( 1 ).toString();
      QList<QByteArray> junks = HandlerHelper::splitLine( queryString.toUtf8() );
      QByteArray mimeType = SearchHelper::extractMimetype( junks, 0 );

      SearchProvider *provider = SearchProviderManager::self()->createSearchProviderForMimetype( mimeType );
      PersistentSearch *persistentSearch = new PersistentSearch( junks, provider );

      mList.insert( identifier, persistentSearch );
    }
    mListMutex.unlock();
  } else
    qDebug() << "PersistentSearchManager: Unable to reload Persistent Searches";
}

PersistentSearchManager::~PersistentSearchManager()
{
  mDatabase.close();
}

PersistentSearchManager* PersistentSearchManager::self()
{
  if ( !mSelf )
    mSelf = new PersistentSearchManager;

  return mSelf;
}

void PersistentSearchManager::addPersistentSearch( const QString &identifier, PersistentSearch *search )
{
  mListMutex.lock();
  if ( mList.values().contains( search ) ) {
    qWarning( "PersistentSearchManager: tried to add the same persistent search twice!" );
    mListMutex.unlock();
    return;
  }

  if ( mList.contains( identifier ) ) {
    qWarning( "PersistentSearchManager: tried to add the same identifier twice!" );
    mListMutex.unlock();
    return;
  }

  /**
   * Insert in local cache.
   */
  mList.insert( identifier, search );


  /**
   * Insert in database.
   */
  QString queryString;
  const QList<QByteArray> searchCriteria = search->searchCriteria();
  for ( int i = 0; i < queryString.count(); ++i )
    queryString += QLatin1Char(' ') + QString::fromUtf8( searchCriteria[ i ] );

  const QString statement = QString( QLatin1String("INSERT INTO PersistentSearches (name, query) VALUES ('%1', '%2') ") )
                                   .arg( identifier, queryString );

  QSqlQuery query( mDatabase );
  if ( !query.exec( statement ) )
    qDebug( "PersistentSearchManager: Unable to remove identifier '%s' from database", qPrintable( identifier ) );

  mListMutex.unlock();
}

void PersistentSearchManager::removePersistentSearch( const QString &identifier )
{
  mListMutex.lock();

  if ( !identifier.contains( identifier ) ) {
    mListMutex.unlock();
    return;
  }

  /**
   * Remove from local cache.
   */
  delete mList.value( identifier );
  mList.remove( identifier );

  /**
   * Remove from database.
   */
  const QString statement = QString( QLatin1String("DELETE FROM PersistentSearches WHERE name = '%1' ") ).arg( identifier );

  QSqlQuery query( mDatabase );
  if ( !query.exec( statement ) )
    qDebug( "PersistentSearchManager: Unable to remove identifier '%s' from database", qPrintable( identifier ) );

  mListMutex.unlock();
}

QList<QByteArray> PersistentSearchManager::uids( const QString &identifier, const DataStore *store ) const
{
  QList<QByteArray> uids;

  mListMutex.lock();
  if ( mList.contains( identifier ) ) {
    PersistentSearch *search = mList.value( identifier );
    uids = search->uids( store );
  }
  mListMutex.unlock();

  return uids;
}

QList<QByteArray> PersistentSearchManager::objects( const QString &identifier, const DataStore *store ) const
{
  QList<QByteArray> objects;

  mListMutex.lock();
  if ( mList.contains( identifier ) ) {
    PersistentSearch *search = mList.value( identifier );
    objects = search->objects( store );
  }
  mListMutex.unlock();

  return objects;
}

CollectionList PersistentSearchManager::collections() const
{
  CollectionList collections;

  QStringList identifiers;
  mListMutex.lock();
  identifiers = mList.keys();
  mListMutex.unlock();

  for ( int i = 0; i < identifiers.count(); ++i )
      collections.append( Collection( QLatin1String("Search/") + identifiers[ i ] ) );

  return collections;
}
