/***************************************************************************
 *   Copyright (C) 2006 by Andreas Gungl <a.gungl@gmx.de>                  *
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

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QtCore/QUuid>
#include <QtCore/QVariant>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlDriver>
#include <QtSql/QSqlError>
#include <QtSql/QSqlField>
#include <QtSql/QSqlQuery>

#include "agentmanagerinterface.h"
#include "resourceinterface.h"
#include "dbinitializer.h"
#include "dbusthread.h"
#include "notificationmanager.h"
#include "tracer.h"

#include "datastore.h"

using namespace Akonadi;

/***************************************************************************
 *   DataStore                                                           *
 ***************************************************************************/
DataStore::DataStore() :
  QObject(),
  m_dbOpened( false ),
  m_inTransaction( false ),
  mNotificationCollector( new NotificationCollector( this ) )
{
  open();

  m_inTransaction = false;
  NotificationManager::self()->connectDatastore( this );
}

DataStore::~DataStore()
{
  close();
}

void DataStore::open()
{
  m_connectionName = QUuid::createUuid().toString() + QString::number( reinterpret_cast<qulonglong>( QThread::currentThread() ) );
  Q_ASSERT( !QSqlDatabase::contains( m_connectionName ) );
#ifdef AKONADI_USE_MYSQL_EMBEDDED
  m_database = QSqlDatabase::addDatabase( QLatin1String("QMYSQL_EMBEDDED"), m_connectionName );
  m_database.setDatabaseName( QLatin1String("akonadi") );
  m_database.setConnectOptions( QString::fromLatin1( "SERVER_DATADIR=%1" ).arg( storagePath() ) );
#endif
#ifdef AKONADI_USE_MYSQL
  m_database = QSqlDatabase::addDatabase( QLatin1String("QMYSQL"), m_connectionName );
  m_database.setUserName( QLatin1String("root") );
  m_database.setDatabaseName( QLatin1String("akonadi") );
#endif
#ifdef AKONADI_USE_SQLITE
  m_database = QSqlDatabase::addDatabase( QLatin1String("QSQLITE"), m_connectionName );
  m_database.setDatabaseName( storagePath() );
#endif

  if ( !m_database.isValid() ) {
    m_dbOpened = false;
    return;
  }
  m_dbOpened = m_database.open();

  if ( !m_dbOpened )
    debugLastDbError( "Cannot open database." );
  else
    qDebug() << "Database akonadi.db opened.";

  Q_ASSERT( m_database.driver()->hasFeature( QSqlDriver::LastInsertId ) );
  //Q_ASSERT( m_database.driver()->hasFeature( QSqlDriver::Transactions ) );
}

void Akonadi::DataStore::close()
{
  if ( inTransaction() )
    rollbackTransaction();

  m_database.close();
  m_database = QSqlDatabase();
  QSqlDatabase::removeDatabase( m_connectionName );
}

void Akonadi::DataStore::init()
{
  DbInitializer initializer( m_database, QLatin1String(":akonadidb.xml") );
  if ( !initializer.run() ) {
    Tracer::self()->error( "DataStore::init()", QString::fromLatin1( "Unable to initialize database: %1" ).arg( initializer.errorMsg() ) );
  }
}


QString DataStore::storagePath()
{
  /**
   * We need the following path for the database directory:
   *   $HOME/.akonadi/db/akonadi/
   */
  const QString akonadiHomeDir = QDir::homePath() + QDir::separator() + QLatin1String(".akonadi") + QDir::separator();
  if ( !QDir( akonadiHomeDir ).exists() ) {
    QDir dir;
    if ( !dir.mkdir( akonadiHomeDir ) )
      Tracer::self()->error( "DataStore::storagePath",
                             QString::fromLatin1( "Unable to create directory '%1'" ).arg( akonadiHomeDir ) );
  }

#ifdef AKONADI_USE_SQLITE
  const QString akonadiPath = akonadiHomeDir + QLatin1String("akonadi.db");
  if ( !QFile::exists( akonadiPath ) ) {
    QFile file( akonadiPath );
    if ( !file.open( QIODevice::WriteOnly ) ) {
      Tracer::self()->error( "DataStore::storagePath", QString::fromLatin1( "Unable to create file '%1'" ).arg( akonadiPath ) );
    } else {
      file.close();
    }
  }
  return akonadiPath;
#else
  const QString dbDataDir = akonadiHomeDir + QLatin1String( "db" ) + QDir::separator();
  if ( !QDir( dbDataDir ).exists() ) {
    QDir dir;
    if ( !dir.mkdir( dbDataDir ) )
      Tracer::self()->error( "DataStore::storagePath",
                             QString::fromLatin1( "Unable to create directory '%1'" ).arg( dbDataDir ) );
  }

  const QString dbDir = dbDataDir + QLatin1String("akonadi");

  if ( !QDir( dbDir ).exists() ) {
    QDir dir;
    if ( !dir.mkdir( dbDir ) )
      Tracer::self()->error( "DataStore::storagePath",
                             QString::fromLatin1( "Unable to create directory '%1'" ).arg( dbDir ) );
  }

  /**
   * Return the data directory and not the database directory
   */
  return dbDataDir;
#endif
}

/* -- High level API -- */
CollectionList Akonadi::DataStore::listCollections( const QString & prefix,
                                                    const QString & mailboxPattern ) const
{
  CollectionList result;

  if ( mailboxPattern.isEmpty() )
    return result;

  // normalize path and pattern
  QString sanitizedPattern( mailboxPattern );
  QString fullPrefix( prefix );
  const bool hasPercent = mailboxPattern.contains( QLatin1Char('%') );
  const bool hasStar = mailboxPattern.contains( QLatin1Char('*') );
  const int endOfPath = mailboxPattern.lastIndexOf( locationDelimiter() ) + 1;

  Resource resource;
  if ( fullPrefix.startsWith( QLatin1Char('#') ) ) {
    int endOfRes = fullPrefix.indexOf( locationDelimiter() );
    QString resourceName;
    if ( endOfRes < 0 ) {
      resourceName = fullPrefix.mid( 1 );
      fullPrefix = QString();
    } else {
      resourceName = fullPrefix.mid( 1, endOfRes - 1 );
      fullPrefix = fullPrefix.right( fullPrefix.length() - endOfRes );
    }

    qDebug() << "listCollections()" << resourceName;
    resource = resourceByName( resourceName.toUtf8() );
    qDebug() << "resource.isValid()" << resource.isValid();
    if ( !resource.isValid() ) {
      result.setValid( false );
      return result;
    }
  }

  if ( !mailboxPattern.startsWith( locationDelimiter() ) && fullPrefix != locationDelimiter() )
    fullPrefix += locationDelimiter();
  fullPrefix += mailboxPattern.left( endOfPath );

  if ( hasPercent )
    sanitizedPattern = QLatin1String("%");
  else if ( hasStar )
    sanitizedPattern = QLatin1String("*");
  else
    sanitizedPattern = mailboxPattern.mid( endOfPath );

  qDebug() << "Resource: " << resource.resource() << " fullPrefix: " << fullPrefix << " pattern: " << sanitizedPattern;

  if ( !fullPrefix.isEmpty() ) {
    result.setValid( false );
  }

  const QList<Location> locations = listLocations( resource );
  foreach( Location l, locations ) {
    const QString location = locationDelimiter() + l.location();
#if 0
    qDebug() << "Location: " << location << " l: " << l << " prefix: " << fullPrefix;
#endif
    const bool atFirstLevel =
      location.lastIndexOf( locationDelimiter() ) == fullPrefix.lastIndexOf( locationDelimiter() );
    if ( location.startsWith( fullPrefix ) ) {
      if ( hasStar || ( hasPercent && atFirstLevel ) ||
           location == fullPrefix + sanitizedPattern ) {
        Collection c( location.right( location.size() -1 ) );
        c.setMimeTypes( MimeType::asCommaSeparatedString( l.mimeTypes() ) );
        result.append( c );
      }
    }
    // Check, if requested folder has been found to distinguish between
    // non-existant folder and empty folder.
    if ( location + locationDelimiter() == fullPrefix || fullPrefix == locationDelimiter() )
      result.setValid( true );
  }

  // list queries (only in global namespace)
  if ( !resource.isValid() ) {
    if ( fullPrefix == locationDelimiter() ) {
      CollectionList persistenSearches = listPersistentSearches();
      if ( !persistenSearches.isEmpty() )
        result.append( Collection( QLatin1String("Search") ) );
      result.setValid( true );
    }
    if ( fullPrefix == QLatin1String("/Search/")  || (fullPrefix == locationDelimiter() && hasStar) ) {
      result += listPersistentSearches();
      result.setValid( true );
    }
  }

  return result;
}

/* --- Flag ---------------------------------------------------------- */
bool DataStore::appendFlag( const QString & name )
{
  if ( !m_dbOpened )
    return false;

  int foundRecs = 0;
  QSqlQuery query( m_database );

  //query.prepare( "SELECT COUNT(*) FROM Flags WHERE name = :name" );
  //query.bindValue( ":name", name );
  QString statement = QString::fromLatin1( "SELECT COUNT(*) FROM Flags WHERE name = '%1'" ).arg( name );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during check before insertion of Flag." );
    return false;
  }

  if ( query.next() )
    foundRecs = query.value( 0 ).toInt();

  if ( foundRecs > 0 ) {
    qDebug() << "Cannot insert flag " << name
             << " because it already exists.";
    return false;
  }

  //query.prepare( "INSERT INTO Flags (name) VALUES (:name)" );
  //query.bindValue( ":name", name );
  statement = QString::fromLatin1( "INSERT INTO Flags (name) VALUES ('%1')" ).arg( name );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during insertion of single Flag." );
    return false;
  }

  return true;
}

bool DataStore::removeFlag( const Flag & flag )
{
  return removeCachePolicy( flag.id() );
}

bool DataStore::removeFlag( int id )
{
  return removeById( id, QLatin1String("Flags") );
}

Flag DataStore::flagById( int id )
{
  if ( !m_dbOpened )
    return Flag();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT id, name FROM Flags WHERE id = :id") );
  query.bindValue( QLatin1String(":id"), id );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of single Flag." );
    return Flag();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single Flag." );
    return Flag();
  }

  int flagId = query.value( 0 ).toInt();
  QString name = query.value( 1 ).toString();

  return Flag( flagId, name );
}

Flag DataStore::flagByName( const QString &name )
{
  if ( !m_dbOpened )
    return Flag();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT id, name FROM Flags WHERE name = :name") );
  query.bindValue( QLatin1String(":name"), name );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of single Flag." );
    return Flag();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single Flag." );
    return Flag();
  }

  int id = query.value( 0 ).toInt();
  QString flagName = query.value( 1 ).toString();

  return Flag( id, flagName );
}

QList<Flag> DataStore::listFlags()
{
  QList<Flag> list;

  if ( !m_dbOpened )
    return list;

  QSqlQuery query( m_database );
  if ( !query.exec( QLatin1String("SELECT id, name FROM Flags") ) ) {
    debugLastQueryError( query, "Error during selection of Flags." );
    return list;
  }

  while ( query.next() ) {
    int id = query.value( 0 ).toInt();
    QString name = query.value( 1 ).toString();
    list.append( Flag( id, name ) );
  }

  return list;
}

/* --- ItemFlags ----------------------------------------------------- */

bool DataStore::setItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  // first delete all old flags of this pim item
  const QString statement = QString::fromLatin1( "DELETE FROM ItemFlags WHERE pimitem_id = %1" ).arg( item.id() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during deletion of ItemFlags." );
    return false;
  }

  // then add the new flags
  for ( int i = 0; i < flags.count(); ++i ) {
    const QString statement = QString::fromLatin1( "INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES ( '%1', '%2' )" )
                                     .arg( flags[ i ].id() ).arg( item.id() );
    if ( !query.exec( statement ) ) {
      debugLastQueryError( query, "Error during adding new ItemFlags." );
      return false;
    }
  }

  mNotificationCollector->itemChanged( item );
  return true;
}

bool DataStore::appendItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  for ( int i = 0; i < flags.count(); ++i ) {
    query.prepare( QLatin1String("SELECT COUNT(*) FROM ItemFlags WHERE flag_id = :flag_id AND pimitem_id = :pimitem_id") );
    query.bindValue( QLatin1String(":flag_id"), flags[ i ].id() );
    query.bindValue( QLatin1String(":pimitem_id"), item.id() );

    if ( !query.exec() ) {
      debugLastQueryError( query, "Error during select on ItemFlags." );
      return false;
    }

    query.next();
    int exists = query.value( 0 ).toInt();

    if ( !exists ) {
      query.prepare( QLatin1String("INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES (:flag_id, :pimitem_id)") );
      query.bindValue( QLatin1String(":flag_id"), flags[ i ].id() );
      query.bindValue( QLatin1String(":pimitem_id"), item.id() );

      if ( !query.exec() ) {
        debugLastQueryError( query, "Error during adding new ItemFlags." );
        return false;
      }
    }
  }

  mNotificationCollector->itemChanged( item );
  return true;
}

bool DataStore::appendItemFlags( int pimItemId, const QList<QByteArray> &flags )
{
    // FIXME Implement me
    return true;
}

bool DataStore::removeItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  for ( int i = 0; i < flags.count(); ++i ) {
    const QString statement = QString::fromLatin1( "DELETE FROM ItemFlags WHERE flag_id = %1 AND pimitem_id = %2" )
                                     .arg( flags[ i ].id() ).arg( item.id() );
    if ( !query.exec( statement ) ) {
      debugLastQueryError( query, "Error during deletion of ItemFlags." );
      return false;
    }
  }

  mNotificationCollector->itemChanged( item );
  return true;
}

QList<Flag> DataStore::itemFlags( const PimItem &item )
{
  QList<Flag> flags;

  if ( !m_dbOpened )
    return flags;

  QSqlQuery query( m_database );
  const QString statement = QString::fromLatin1( "SELECT Flags.id, Flags.name FROM Flags, ItemFlags WHERE Flags.id = ItemFlags.flag_id AND ItemFlags.pimitem_id = %1" )
                                   .arg( item.id() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during selection of ItemFlags." );
    return flags;
  }

  while (query.next()) {
    int id = query.value( 0 ).toInt();
    QString name = query.value( 1 ).toString();
    flags.append( Flag( id, name ) );
  }

  return flags;
}

/* --- CachePolicy --------------------------------------------------- */
bool DataStore::appendCachePolicy( const QString & policy )
{
  if ( !m_dbOpened )
    return false;

  int foundRecs = 0;
  QSqlQuery query( m_database );

  query.prepare( QLatin1String("SELECT COUNT(*) FROM CachePolicies WHERE name = :name") );
  query.bindValue( QLatin1String(":name"), policy );

  if ( !query.exec() ) {
    return false;
    debugLastQueryError( query, "Error during check before insertion of CachePolicy." );
  }

  if ( !query.next() ) {
    return false;
    debugLastQueryError( query, "Error during check before insertion of CachePolicy." );
  }

  foundRecs = query.value( 0 ).toInt();

  if ( foundRecs > 0 ) {
    qDebug() << "Cannot insert policy " << policy
             << " because it already exists.";
    return false;
  }

  query.prepare( QLatin1String("INSERT INTO CachePolicies (name) VALUES (:name)") );
  query.bindValue( QLatin1String(":name"), policy );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single CachePolicy." );
    return false;
  }

  return true;
}

bool DataStore::removeCachePolicy( const CachePolicy & policy )
{
  return removeCachePolicy( policy.id() );
}

bool DataStore::removeCachePolicy( int id )
{
  return removeById( id, QLatin1String("CachePolicies") );
}

CachePolicy DataStore::cachePolicyById( int id )
{
  if ( !m_dbOpened )
    return CachePolicy();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT id, name FROM CachePolicies WHERE id = :id") );
  query.bindValue( QLatin1String(":id"), id );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of single CachePolicy." );
    return CachePolicy();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single CachePolicy." );
    return CachePolicy();
  }

  int policyId = query.value( 0 ).toInt();
  QString policy = query.value( 1 ).toString();

  return CachePolicy( policyId, policy );
}

QList<CachePolicy> DataStore::listCachePolicies()
{
  if ( !m_dbOpened )
    return QList<CachePolicy>();

  QSqlQuery query( m_database );
  if ( !query.exec( QLatin1String("SELECT id, name FROM CachePolicies" ) ) ) {
    debugLastQueryError( query, "Error during selection of CachePolicies." );
    return QList<CachePolicy>();
  }

  QList<CachePolicy> list;
  while ( query.next() ) {
    int id = query.value( 0 ).toInt();
    QString policy = query.value( 1 ).toString();

    list.append( CachePolicy( id, policy ) );
  }

  return list;
}

/* --- ItemMetaData--------------------------------------------------- */
/*
bool appendItemMetaData( const QString & metadata, const MetaType & metatype );
bool removeItemMetaData( const ItemMetaData & metadata );
bool removeItemMetaData( int id );
ItemMetaData * itemMetaDataById( int id );
QList<ItemMetaData> * listItemMetaData();
QList<ItemMetaData> * listItemMetaData( const MetaType & metatype );
*/

/* --- Location ------------------------------------------------------ */
bool DataStore::appendLocation( const QString & location,
                                const Resource & resource,
                                int *insertId )
{
  if ( !m_dbOpened )
    return false;

  int foundRecs = 0;
  QSqlQuery query( m_database );

  query.prepare( QLatin1String("SELECT COUNT(*) FROM Locations WHERE uri = :uri") );
  query.bindValue( QLatin1String(":uri"), location );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during check before insertion of Location." );
    return false;
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during check before insertion of Location." );
    return false;
  }

  foundRecs = query.value( 0 ).toInt();

  if ( foundRecs > 0 ) {
    qDebug() << "Cannot insert location " << location
             << " because it already exists.";
    return false;
  }

  QString statement = QString::fromLatin1( "INSERT INTO Locations (uri, cachepolicy_id, "
                               "resource_id, exists_count, recent_count, "
                               "unseen_count, first_unseen, uid_validity) "
                               "VALUES (:uri, NULL, :resource_id, 0, 0, 0, 0, 0 )" );
  query.prepare( statement );
  query.bindValue( QLatin1String(":uri"), location );
  query.bindValue( QLatin1String(":resource_id"), resource.id() );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single Location." );
    return false;
  }

  int id = lastInsertId( query );
  if ( insertId )
      *insertId = id;

  mNotificationCollector->collectionAdded( location, resource.resource().toLatin1() );
  return true;
}

bool DataStore::appendLocation( const QString & location,
                                const Resource & resource,
                                const CachePolicy & policy )
{
  if ( !m_dbOpened )
    return false;

  int foundRecs = 0;
  QSqlQuery query( m_database );

  query.prepare( QLatin1String("SELECT COUNT(*) FROM Locations WHERE uri = :uri") );
  query.bindValue( QLatin1String(":uri"), location );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during check before insertion of Location." );
    return false;
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during check before insertion of Location." );
    return false;
  }

  foundRecs = query.value( 0 ).toInt();

  if ( foundRecs > 0 ) {
    qDebug() << "Cannot insert location " << location
             << " because it already exists.";
    return false;
  }

  query.prepare( QLatin1String("INSERT INTO Locations (uri, cachepolicy_id, resource_id) "
                 "VALUES (:uri, :policy, :resource)") );
  query.bindValue( QLatin1String(":uri"), location );
  query.bindValue( QLatin1String(":policy"), policy.id() );
  query.bindValue( QLatin1String(":resource"), resource.id() );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single Location." );
    return false;
  }

  mNotificationCollector->collectionAdded( location, resource.resource().toLatin1() );
  return true;
}

bool DataStore::removeLocation( const Location & location )
{
  mNotificationCollector->collectionRemoved( location );
  return removeById( location.id(), QLatin1String("Locations") );
}

bool DataStore::removeLocation( int id )
{
  return removeLocation( locationById( id ) );
}

bool Akonadi::DataStore::cleanupLocation(const Location & location)
{
  // delete the content
  QList<QByteArray> seq;
  seq << "0:*";
  QList<PimItem> items = matchingPimItemsByUID( seq, location );
  foreach ( PimItem item, items )
    cleanupPimItem( item );

  // delete location mimetypes
  removeMimeTypesForLocation( location.id() );

  // delete the location itself
  return removeLocation( location );
}

static void addToUpdateAssignments( QStringList & l, int change, const QString & name )
{
    if ( change > 0 )
        // return a = a + n
        l.append( name + QLatin1String(" = ") + name + QLatin1String(" + ") + QString::number( change ) );
    else if ( change < 0 )
        // return a = a - |n|
        l.append( name + QLatin1String(" = ") + name + QLatin1String(" - ") + QString::number( -change ) );
}

bool DataStore::updateLocationCounts( const Location & location, int existsChange,
                                      int recentChange, int unseenChange )
{
    if ( existsChange == 0 && recentChange == 0 && unseenChange == 0 )
        return true; // there's nothing to do

    QSqlQuery query( m_database );
    if ( m_dbOpened ) {
        QStringList assignments;
        addToUpdateAssignments( assignments, existsChange, QLatin1String("exists_count") );
        addToUpdateAssignments( assignments, recentChange, QLatin1String("recent_count") );
        addToUpdateAssignments( assignments, unseenChange, QLatin1String("unseen_count") );
        QString q = QString::fromLatin1( "UPDATE Locations SET %1 WHERE id = :id" )
                    .arg( assignments.join(QLatin1String(",")) );
        qDebug() << "Executing SQL query " << q;
        query.prepare( q );
        query.bindValue( QLatin1String(":id"), location.id() );
        if ( query.exec() )
            return true;
        else
            debugLastQueryError( query, "Error while updating the counts of a single Location." );
    }
    return false;
}

bool DataStore::changeLocationPolicy( const Location & location,
                                      const CachePolicy & policy )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  query.prepare( QLatin1String("UPDATE Locations SET cachepolicy_id = :policy WHERE id = :id") );
  query.bindValue( QLatin1String(":policy"), policy.id() );
  query.bindValue( QLatin1String(":id"), location.id() );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during setting the cache policy of a single Location." );
    return false;
  }

  return true;
}

bool DataStore::resetLocationPolicy( const Location & location )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  query.prepare( QLatin1String("UPDATE Locations SET cachepolicy_id = NULL WHERE id = :id") );
  query.bindValue( QLatin1String(":id"), location.id() );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during reset of the cache policy of a single Location." );
    return false;
  }

  return true;
}

bool Akonadi::DataStore::renameLocation(const Location & location, const QString & newName)
{
  if ( !m_dbOpened )
    return false;

  mNotificationCollector->collectionRemoved( location );

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("UPDATE Locations SET uri = :name WHERE id = :id") );
  query.bindValue( QLatin1String(":id"), location.id() );
  query.bindValue( QLatin1String(":name"), newName );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during renaming of a single location." );
    return false;
  }

  mNotificationCollector->collectionAdded( newName );
  return true;
}

Location DataStore::locationById( int id ) const
{
  if ( !m_dbOpened )
    return Location();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE id = :id") );
  query.bindValue( QLatin1String(":id"), id );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of single Location." );
    return Location();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single Location." );
    return Location();
  }

  int locationId = query.value( 0 ).toInt();
  QString uri = query.value( 1 ).toString();
  int policy = query.value( 2 ).toInt();
  int resource = query.value( 3 ).toInt();

  return Location( locationId, uri, policy, resource );
}

QList<MimeType> DataStore::mimeTypesForLocation( int id ) const
{
  if ( !m_dbOpened )
    return QList<MimeType>();

  QSqlQuery query( m_database );
  const QString statement = QString::fromLatin1( "SELECT mimetype_id FROM LocationMimeTypes WHERE location_id = %1" ).arg( id );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during selection of Locations." );
    return QList<MimeType>();
  }

  QList<MimeType> mimeTypes;
  while ( query.next() ) {
    int id = query.value( 0 ).toInt();
    MimeType mimeType = mimeTypeById( id );

    if ( mimeType.isValid() )
      mimeTypes.append( mimeType );
  }

  return mimeTypes;
}

bool DataStore::appendMimeTypeForLocation( int locationId, const QString & mimeType )
{
  //qDebug() << "DataStore::appendMimeTypeForLocation( " << locationId << ", '" << mimeType << "' )";
  int mimeTypeId;
  MimeType m = mimeTypeByName( mimeType );
  if ( !m.isValid() ) {
    // the MIME type doesn't exist, so we have to add it to the db
    if ( !appendMimeType( mimeType, &mimeTypeId ) )
      return false;
  } else {
    mimeTypeId = m.id();
  }

  return appendMimeTypeForLocation( locationId, mimeTypeId );
}

bool DataStore::appendMimeTypeForLocation( int locationId, int mimeTypeId )
{
  if ( !m_dbOpened )
    return false;

  //qDebug() << "DataStore::appendMimeTypeForLocation( " << locationId << ", '" << mimeTypeId << "' )";
  QSqlQuery query( m_database );
  query.prepare( QLatin1String( "SELECT COUNT(*) FROM LocationMimeTypes WHERE location_id = :locationId AND mimetype_id = :mimetypeId" ) );
  query.bindValue( QLatin1String(":locationId"), locationId );
  query.bindValue( QLatin1String(":mimetypeId"), mimeTypeId );

  if ( !query.exec() ) {
    debugLastDbError( "Error during check before insertion of LocationMimeType." );
    return false;
  }

  if ( !query.next() ) {
    debugLastDbError( "Error during check before insertion of LocationMimeType." );
    return false;
  }

  int foundRecs = query.value( 0 ).toInt();

  if ( foundRecs > 0 ) {
    qDebug() << "Cannot insert location-mime type ( " << locationId
             << ", " << mimeTypeId << " ) because it already exists.";
    return false;
  }

  query.prepare( QLatin1String( "INSERT INTO LocationMimeTypes (location_id, mimetype_id) VALUES (:locationId, :mimeTypeId)" ) );
  query.bindValue( QLatin1String(":locationId"), locationId );
  query.bindValue( QLatin1String(":mimeTypeId"), mimeTypeId );

  if ( !query.exec() ) {
    debugLastDbError( "Error during insertion of single LocationMimeType." );
    debugLastQueryError( query, "Error during insertion of single LocationMimeType." );
    return false;
  }

  return true;
}


bool Akonadi::DataStore::removeMimeTypesForLocation(int locationId)
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );
  QString statement = QString::fromLatin1( "DELETE FROM LocationMimeTypes WHERE location_id = %1" ).arg( locationId );
  return query.exec( statement );
}


QList<Location> DataStore::listLocations( const Resource & resource ) const
{
  if ( !m_dbOpened )
    return QList<Location>();

  QSqlQuery query( m_database );
  // query.prepare( "SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE resource_id = :id" );
  // query.bindValue( ":id", resource.id() );
  QString statement = QString::fromLatin1( "SELECT id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity FROM Locations" );

  if ( resource.isValid() )
    statement += QString::fromLatin1( " WHERE resource_id = %1" ).arg( resource.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during selection of Locations from a Resource." );
    return QList<Location>();
  }

  QList<Location> locations;
  while ( query.next() ) {
    Location location;
    location.fillFromQuery( query );
    location.setMimeTypes( mimeTypesForLocation( location.id() ) );
    locations.append( location );
  }

  return locations;
}

/* --- MimeType ------------------------------------------------------ */
bool DataStore::appendMimeType( const QString & mimetype, int *insertId )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  const QString statement = QString::fromLatin1( "SELECT COUNT(*) FROM MimeTypes WHERE mime_type = \"%1\"" )
                                   .arg( mimetype );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during check before insertion of MimeType." );
    return false;
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during check before insertion of MimeType." );
    return false;
  }

  int foundRecs = query.value( 0 ).toInt();

  if ( foundRecs > 0 ) {
    qDebug() << "Cannot insert mimetype " << mimetype
             << " because it already exists.";
    return false;
  }

  query.prepare( QLatin1String("INSERT INTO MimeTypes (mime_type) VALUES (:type)") );
  query.bindValue( QLatin1String(":type"), mimetype );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single MimeType." );
    return false;
  }

  if ( insertId )
      *insertId = lastInsertId( query );

  return true;
}

bool DataStore::removeMimeType( const MimeType & mimetype )
{
  return removeMimeType( mimetype.id() );
}

bool DataStore::removeMimeType( int id )
{
  return removeById( id, QLatin1String("MimeTypes") );
}

MimeType DataStore::mimeTypeById( int id ) const
{
  if ( !m_dbOpened )
    return MimeType();

  QSqlQuery query( m_database );
  const QString statement = QString::fromLatin1( "SELECT id, mime_type FROM MimeTypes WHERE id = %1" ).arg( id );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during selection of single MimeType." );
    return MimeType();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single MimeType." );
    return MimeType();
  }

  int mimeTypeId = query.value( 0 ).toInt();
  QString type = query.value( 1 ).toString();

  return MimeType( mimeTypeId, type );
}

MimeType DataStore::mimeTypeByName( const QString & name ) const
{
  if ( !m_dbOpened )
    return MimeType();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String( "SELECT id, mime_type FROM MimeTypes WHERE mime_type = :name" ) );
  query.bindValue( QLatin1String(":name"), name );

  if ( !query.exec() ) {
    debugLastDbError( "Error during selection of single MimeType." );
    return MimeType();
  }

  if ( !query.next() ) {
    debugLastDbError( "Error during selection of single MimeType." );
    return MimeType();
  }

  int id = query.value( 0 ).toInt();
  QString type = query.value( 1 ).toString();

  return MimeType( id, type );
}

QList<MimeType> DataStore::listMimeTypes()
{
  if ( !m_dbOpened )
    return QList<MimeType>();

  QSqlQuery query( m_database );
  if ( !query.exec( QLatin1String("SELECT id, mime_type FROM MimeTypes") ) ) {
    debugLastQueryError( query, "Error during selection of MimeTypes." );
    return QList<MimeType>();
  }

  QList<MimeType> mimeTypes;
  while ( query.next() ) {
    int id = query.value( 0 ).toInt();
    QString type = query.value( 1 ).toString();

    mimeTypes.append( MimeType( id, type ) );
  }

  return mimeTypes;
}


/* --- MetaType ------------------------------------------------------ */
bool DataStore::appendMetaType( const QString & metatype, const MimeType & mimetype )
{
  // TODO implement
  return false;
}

bool DataStore::removeMetaType( const MetaType & metatype )
{
  // TODO implement
  return false;
}

bool DataStore::removeMetaType( int id )
{
  // TODO implement
  return false;
}

MetaType DataStore::metaTypeById( int id )
{
  // TODO implement
  MetaType m;
  m.setId( id );
  m.setMetaType( QLatin1String("dummyMetaType") );
  m.setMimeTypeId( 1 );
  return m;
}

QList<MetaType> DataStore::listMetaTypes()
{
  // TODO implement
  QList<MetaType> list;
  list.append( metaTypeById( 1 ) );
  return list;
}

QList<MetaType> DataStore::listMetaTypes( const MimeType & mimetype )
{
  // TODO implement
  QList<MetaType> list;
  // let's fake it for now
  if ( mimetype.id() == 1 )
    list.append( this->metaTypeById( 1 ) );
  return list;
}


/* --- PimItem ------------------------------------------------------- */
bool DataStore::appendPimItem( const QByteArray & data,
                               const MimeType & mimetype,
                               const Location & location,
                               const QDateTime & dateTime,
                               const QByteArray & remote_id,
                               int *insertId )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  if ( dateTime.isValid() ) {
      query.prepare( QLatin1String( "INSERT INTO PimItems (remote_id, data, location_id, mimetype_id, datetime ) "
                                    "VALUES (:remote_id, :data, :location_id, :mimetype_id, :datetime)") );
      query.bindValue( QLatin1String(":datetime"), dateTimeFromQDateTime( dateTime ) );
  } else {
      // the database will use the current date/time for datetime
      query.prepare( QLatin1String( "INSERT INTO PimItems (remote_id, data, location_id, mimetype_id) "
                                    "VALUES (:remote_id, :data, :location_i, :mimetype_id)") );
  }
  query.bindValue( QLatin1String(":remote_id"), QString::fromUtf8(remote_id) );
  query.bindValue( QLatin1String(":data"), data );
  query.bindValue( QLatin1String(":location_id"), location.id() );
  query.bindValue( QLatin1String(":mimetype_id"), mimetype.id() );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single PimItem." );
    return false;
  }

  int id = lastInsertId( query );
  if ( insertId )
      *insertId = id;

  mNotificationCollector->itemAdded( pimItemById( id ), location, mimetype.mimeType().toLatin1() );

  return true;
}

bool Akonadi::DataStore::updatePimItem(const PimItem & pimItem, const QByteArray & data)
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("UPDATE PimItems SET data = :data WHERE id = :id") );
  query.bindValue( QLatin1String(":data"), data );
  query.bindValue( QLatin1String(":id"), pimItem.id() );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single PimItem." );
    return false;
  }

  mNotificationCollector->itemChanged( pimItem );
  return true;
}

bool Akonadi::DataStore::updatePimItem(const PimItem & pimItem, const Location & destination)
{
  if ( !m_dbOpened || !pimItem.isValid() || !destination.isValid() )
    return false;

  Location source = locationById( pimItem.locationId() );
  if ( !source.isValid() )
    return false;
  mNotificationCollector->collectionChanged( source );

  QSqlQuery query( m_database );
  QString statement = QString::fromLatin1( "UPDATE PimItems SET location_id = %1 WHERE id = %2" ).
      arg( destination.id() ).arg( pimItem.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during moving of a single PimItem." );
    return false;
  }

  mNotificationCollector->collectionChanged( destination );
  return true;
}

bool DataStore::cleanupPimItem( const PimItem &item )
{
  if ( !m_dbOpened || !item.isValid() )
    return false;


  // generate the notification before actually removing the data
  mNotificationCollector->itemRemoved( item );

  QSqlQuery query( m_database );

  QString statement = QString::fromLatin1( "DELETE FROM ItemMetaData WHERE pimitem_id = %1" ).arg( item.id() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during deletion of item meta data." );
    return false;
  }

  statement = QString::fromLatin1( "DELETE FROM ItemFlags WHERE pimitem_id = %1" ).arg( item.id() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during deletion of item flags." );
    return false;
  }

  statement = QString::fromLatin1( "DELETE FROM Parts WHERE pimitem_id = %1" ).arg( item.id() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during deletion of item parts." );
    return false;
  }

  statement = QString::fromLatin1( "DELETE FROM PimItems WHERE id = %1" ).arg( item.id() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during deletion of a single item." );
    return false;
  }

  return true;
}

bool DataStore::cleanupPimItems( const Location &location )
{
  if ( !m_dbOpened || !location.isValid() )
    return false;

  const QString statement = QString::fromLatin1( "SELECT ItemFlags.pimitem_id FROM Flags, ItemFlags, PimItems "
                                     "WHERE Flags.name = '\\Deleted' AND "
                                     "ItemFlags.flag_id = Flags.id AND "
                                     "ItemFlags.pimitem_id = PimItems.id AND "
                                     "PimItems.location_id = %1" ).arg( location.id() );

  QSqlQuery query( m_database );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during cleaning up pim items." );
    return false;
  }

  QList<PimItem> pimItems;
  while ( query.next() ) {
    PimItem item;
    item.setId( query.value( 0 ).toInt() );

    pimItems.append( item );
  }

  bool ok = true;
  for ( int i = 0; i < pimItems.count(); ++i )
    ok = ok && cleanupPimItem( pimItems[ i ] );

  return ok;
}

int DataStore::pimItemPosition( const PimItem &item )
{
  if ( !m_dbOpened || !item.isValid() )
    return -1;

  QSqlQuery query( m_database );

  const QString statement = QString::fromLatin1( "SELECT id FROM PimItems WHERE location_id = %1" ).arg( item.locationId() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during selection of pim item position." );
    return -1;
  }

  int i = 1;
  while ( query.next() ) {
    if ( item.id() == query.value( 0 ).toInt() )
      return i;

    ++i;
  }

  return -1;
}

QString fieldNameForDataType( FetchQuery::Type type )
{
  return QLatin1String("data");
}

QByteArray Akonadi::DataStore::retrieveDataFromResource( const QByteArray &uid, const QByteArray& remote_id,
                                                         int locationid, FetchQuery::Type type )
{
  // TODO: error handling
  qDebug() << "retrieveDataFromResource()" << uid;
  Location l = locationById( locationid );
  Resource r = resourceById( l.resourceId() );

  // call the resource

  // Use the interface if we are in main thread, the DBusThread proxy otherwise
  if ( QThread::currentThread() == QCoreApplication::instance()->thread() ) {
      org::kde::Akonadi::Resource *interface =
                  new org::kde::Akonadi::Resource( QLatin1String("org.kde.Akonadi.Resource.") + r.resource(),
                                                   QLatin1String("/"), QDBusConnection::sessionBus(), this );

      if ( !interface || !interface->isValid() ) {
         qDebug() << QString::fromLatin1( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                                        .arg( r.resource(), interface ? interface->lastError().message() : QString() );
         return QByteArray();
      }
      bool ok = interface->requestItemDelivery( QString::fromLatin1(uid), QString::fromUtf8(remote_id), l.location(), type );
  } else {
    QList<QVariant> arguments;
    arguments << QString::fromLatin1( uid ) << QString::fromUtf8( remote_id ) << l.location() << type;

    DBusThread *dbusThread = static_cast<DBusThread*>( QThread::currentThread() );

    const QList<QVariant> result = dbusThread->callDBus( QLatin1String( "org.kde.Akonadi.Resource." ) + r.resource(),
                                                         QLatin1String( "/" ),
                                                         QLatin1String( "org.kde.Akonadi.Resource" ),
                                                         QLatin1String( "requestItemDelivery" ), arguments );

    // do something with result...
    qDebug() << "got dbus response: " << result;
  }
  qDebug() << "returned from requestItemDelivery()";

  // get the delivered item
  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT data FROM PimItems WHERE id = :id") );
  query.bindValue( QLatin1String(":id"), uid.toInt() );
  if ( query.exec() && query.next() )
    return query.value( 0 ).toByteArray();

  return QByteArray();
}


PimItem Akonadi::DataStore::pimItemById( int id, FetchQuery::Type type )
{
  if ( !m_dbOpened )
    return PimItem();

  const QString field = fieldNameForDataType( type );
  QSqlQuery query( m_database );
  query.prepare( QString::fromLatin1( "SELECT id, %1, location_id, mimetype_id, datetime, remote_id FROM PimItems WHERE id = :id" ).arg( field ) );
  query.bindValue( QLatin1String(":id"), id );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of single Location." );
    return PimItem();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single Location." );
    return PimItem();
  }

  int pimItemId = query.value( 0 ).toInt();
  int location = query.value( 2 ).toInt();
  int mimetype = query.value( 3 ).toInt();
  QByteArray remote_id = query.value( 5 ).toByteArray();
  QDateTime dateTime = dateTimeToQDateTime( query.value( 4 ).toByteArray() );
  QByteArray data = query.value( 1 ).toByteArray();
  if ( data.isEmpty() && type == FetchQuery::AllType )
      data = retrieveDataFromResource( QByteArray::number( id ), remote_id, location, type );

  return PimItem( pimItemId, remote_id, data, location, mimetype, dateTime );
}

PimItem DataStore::pimItemById( int id )
{
  return pimItemById( id, FetchQuery::FastType );
}

QList<PimItem> DataStore::listPimItems( const MimeType & mimetype,
                                        const Location & location )
{
  // TODO implement
  QList<PimItem> list;
  list.append( pimItemById( 1 ) );
  return list;
}

QList<PimItem> DataStore::listPimItems( const Location & location, const Flag &flag )
{
  if ( !m_dbOpened )
    return QList<PimItem>();

  QSqlQuery query( m_database );

  QString statement = QString::fromLatin1( "SELECT PimItems.id, PimItems.data, PimItems.mimetype_id,"
                               "PimItems.datetime, PimItems.remote_id, PimItems.location_id"
                               " FROM PimItems, ItemFlags WHERE "
                               "ItemFlags.pimitem_id = PimItems.id AND ItemFlags.flag_id = %1" ).arg( flag.id() );
  if ( location.isValid() )
    statement += QString::fromLatin1( " AND location_id = %1" ).arg( location.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "DataStore::listPimItems" );
    return QList<PimItem>();
  }

  QList<PimItem> pimItems;
  while ( query.next() ) {
    pimItems.append( PimItem( query.value( 0 ).toInt(),
                              query.value( 4 ).toByteArray(),
                              query.value( 1 ).toByteArray(),
                              query.value( 5 ).toInt(), query.value( 2 ).toInt(),
                              dateTimeToQDateTime( query.value( 3 ).toByteArray() ) ) );
  }

  return pimItems;
}

int DataStore::highestPimItemId() const
{
  if ( !m_dbOpened )
    return -1;

  QSqlQuery query( m_database );
  const QString statement = QString::fromLatin1( "SELECT MAX(id) FROM PimItems" );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "DataStore::highestPimItemId" );
    return -1;
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "DataStore::highestPimItemId" );
    return -1;
  }

  return query.value( 0 ).toInt();
}

int DataStore::highestPimItemCountByLocation( const Location &location )
{
  if ( !m_dbOpened || !location.isValid() )
    return -1;

  QSqlQuery query( m_database );
  const QString statement = QString::fromLatin1( "SELECT COUNT(*) AS count FROM PimItems WHERE location_id = %1" ).arg( location.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "DataStore::highestPimItemCountByLocation" );
    return -1;
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "DataStore::highestPimItemCountByLocation" );
    return -1;
  }

  return query.value( 0 ).toInt() + 1;
}


QList<PimItem> Akonadi::DataStore::fetchMatchingPimItemsByUID( const FetchQuery &query, const Location& l )
{
    return matchingPimItemsByUID( query.sequences(), /*query.type() HACK*/ FetchQuery::AllType, l );
}

QList<PimItem> DataStore::matchingPimItemsByUID( const QList<QByteArray> &sequences,
                                                 FetchQuery::Type type,
                                                 const Location& location )
{
  if ( !m_dbOpened )
    return QList<PimItem>();

  int highestEntry = highestPimItemId();
  if ( highestEntry == -1 ) {
    qDebug( "DataStore::matchingPimItems: Invalid highest entry number '%d' ", highestEntry );
    return QList<PimItem>();
  }

  QStringList statementParts;
  for ( int i = 0; i < sequences.count(); ++i ) {
    if ( sequences[ i ].contains( ':' ) ) {
      QList<QByteArray> pair = sequences[ i ].split( ':' );
      const QString left( QString::fromLatin1( pair[ 0 ] ) );
      const QString right( QString::fromLatin1( pair[ 1 ] ) );

      if ( left == QLatin1String("*") && right == QLatin1String("*") ) {
        statementParts.append( QString::fromLatin1( "id = %1" ).arg( QString::number( highestEntry ) ) );
      } else if ( left == QLatin1String("*") ) {
        statementParts.append( QString::fromLatin1( "(id >=1 AND id <= %1)" ).arg( right ) );
      } else if ( right == QLatin1String("*") ) {
        statementParts.append( QString::fromLatin1( "(id >=%1 AND id <= %2)" ).arg( left ).arg( highestEntry ) );
      } else {
        statementParts.append( QString::fromLatin1( "(id >=%1 AND id <= %2)" ).arg( left, right ) );
      }
    } else {
      statementParts.append( QString::fromLatin1( "id = %1" ).arg( QString::fromLatin1( sequences[ i ] ) ) );
    }
  }

  QString statement = QString::fromLatin1( "SELECT id FROM PimItems WHERE (%1)" )
      .arg( statementParts.join( QLatin1String(" OR ") ) );
  if ( location.isValid() ) {
     statement += QString::fromLatin1( " AND location_id = %1" ).arg( location.id() );
  }

  QSqlQuery query( m_database );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "DataStore::matchingPimItemsBySequenceNumbers" );
    return QList<PimItem>();
  }

  QList<PimItem> pimItems;
  while ( query.next() ) {
    PimItem item = pimItemById( query.value( 0 ).toInt(), type );
    if ( !item.isValid() ) {
      qDebug( "DataStore::matchingPimItems: Invalid uid '%d' returned by search ", query.value( 0 ).toInt() );
      return QList<PimItem>();
    }

    pimItems.append( item );
  }

  return pimItems;

}

QList<PimItem> DataStore::matchingPimItemsByUID( const QList<QByteArray> &sequences,
                                                 const Location & location )
{
  return matchingPimItemsByUID( sequences, FetchQuery::FastType, location );
}


QList<PimItem> Akonadi::DataStore::fetchMatchingPimItemsBySequenceNumbers( const FetchQuery & query,
                                                                           const Location & location )
{
  return matchingPimItemsBySequenceNumbers( query.sequences(), location, FetchQuery::FastType );
}

QList<PimItem> DataStore::matchingPimItemsBySequenceNumbers( const QList<QByteArray> &sequences,
                                                             const Location &location )
{
  return matchingPimItemsBySequenceNumbers( sequences, location, FetchQuery::FastType );
}

QList<PimItem> DataStore::matchingPimItemsBySequenceNumbers( const QList<QByteArray> &sequences,
                                                             const Location &location,
                                                             FetchQuery::Type type )
{
  if ( !m_dbOpened || !location.isValid() )
    return QList<PimItem>();

  QSqlQuery query( m_database );
  const QString statement =
      QString::fromLatin1( "SELECT id FROM PimItems WHERE location_id = %1" ).arg( location.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "DataStore::matchingPimItemsBySequenceNumbers" );
    return QList<PimItem>();
  }

  int highestEntry = highestPimItemCountByLocation( location );
  if ( highestEntry == -1 ) {
    qDebug( "DataStore::matchingPimItemsBySequenceNumbers: Invalid highest entry number '%d' ", highestEntry );
    return QList<PimItem>();
  }

  // iterate over the whole query to make seek possible.
  while ( query.next() ) {}

  QList<PimItem> pimItems;
  for ( int i = 0; i < sequences.count(); ++i ) {
    if ( sequences[ i ].contains( ':' ) ) {
      int min = 0;
      int max = 0;

      QList<QByteArray> pair = sequences[ i ].split( ':' );
      if ( pair[ 0 ] == "*" && pair[ 1 ] == "*" ) {
        min = max = highestEntry - 1;
      } else if ( pair[ 0 ] == "*" ) {
        min = 0;
        max = pair[ 1 ].toInt();
      } else if ( pair[ 1 ] == "*" ) {
        min = pair[ 0 ].toInt();
        max = highestEntry - 1;
      } else {
        min = pair[ 0 ].toInt();
        max = pair[ 1 ].toInt();
      }

      if ( min < 1 )
        min = 1;

      if ( max < 1 )
        max = 1;

      if ( max < min )
        qSwap( max, min );

      // transform from imap index to query index
      min--; max--;

      for ( int i = min; i <= max; ++i ) {
        if ( !query.seek( i ) ) {
          qDebug( "DataStore::matchingPimItemsBySequenceNumbers: Unable to seek at position '%d' ", i );
          return QList<PimItem>();
        }

        PimItem item = pimItemById( query.value( 0 ).toInt(), type );
        if ( !item.isValid() ) {
          qDebug( "DataStore::matchingPimItemsBySequenceNumbers: Invalid uid '%d' returned by search ", query.value( 0 ).toInt() );
          return QList<PimItem>();
        }

        pimItems.append( item );
      }
    } else {

      int pos = 0;
      if ( sequences[ i ] == "*" )
        pos = query.size() - 1;
      else
        pos = sequences[ i ].toInt();

      if ( pos < 1 )
        pos = 1;

      // transform from imap index to query index
      pos--;

      if ( !query.seek( pos ) ) {
        qDebug( "DataStore::matchingPimItemsBySequenceNumbers: Unable to seek at position '%d' ", pos );
        return QList<PimItem>();
      }

      PimItem item = pimItemById( query.value( 0 ).toInt(), type );
      if ( !item.isValid() ) {
        qDebug( "DataStore::matchingPimItemsBySequenceNumbers: Invalid uid '%d' returned by search ", query.value( 0 ).toInt() );
        return QList<PimItem>();
      }

      pimItems.append( item );
    }
  }

  return pimItems;
}


/* --- Resource ------------------------------------------------------ */
bool DataStore::appendResource( const QString & resource,
                                const CachePolicy & policy )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT COUNT(*) FROM Resources WHERE name = :name") );
  query.bindValue( QLatin1String(":name"), resource );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during check before insertion of Resource." );
    return false;
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during check before insertion of Resource." );
    return false;
  }

  int foundRecs = query.value( 0 ).toInt();

  if ( foundRecs > 0) {
    qDebug() << "Cannot insert resource " << resource
             << " because it already exists.";
    return false;
  }

  query.prepare( QLatin1String("INSERT INTO Resources (name, cachepolicy_id) "
                 "VALUES (:name, :policy)") );
  query.bindValue( QLatin1String(":name"), resource );
  query.bindValue( QLatin1String(":policy"), policy.id() );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single Resource." );
    return false;
  }

  return true;
}

bool DataStore::removeResource( const Resource & resource )
{
  return removeResource( resource.id() );
}

bool DataStore::removeResource( int id )
{
  return removeById( id, QLatin1String("Resources") );
}

Resource DataStore::resourceById( int id )
{
  if ( !m_dbOpened )
    return Resource();

  QSqlQuery query( m_database );
  QString statement = QString::fromLatin1( "SELECT id, name, cachepolicy_id FROM Resources WHERE id = %1" ).arg( id );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during selection of single Resource." );
    return Resource();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single Resource." );
    return Resource();
  }

  int resourceId = query.value( 0 ).toInt();
  QString name = query.value( 1 ).toString();
  int id_res = query.value( 0 ).toInt();

  return Resource( resourceId, name, id_res );
}

const Resource DataStore::resourceByName( const QByteArray& name ) const
{
  if ( !m_dbOpened )
    return Resource();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT id, name, cachepolicy_id FROM Resources WHERE name = :name") );
  query.bindValue( QLatin1String(":name"), QString::fromUtf8(name) );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of single Resource." );
    return Resource();
  }

  if ( !query.next() ) {
    debugLastQueryError( query, "Error during selection of single Resource." );
    return Resource();
  }

  int id = query.value( 0 ).toInt();
  QString resourceName = query.value( 1 ).toString();
  int id_res = query.value( 0 ).toInt();

  return Resource( id, resourceName, id_res );
}

QList<Resource> DataStore::listResources() const
{
  if ( !m_dbOpened )
    return QList<Resource>();

  QSqlQuery query( m_database );
  if ( !query.exec( QLatin1String("SELECT id, name, cachepolicy_id FROM Resources") ) ) {
    debugLastQueryError( query, "Error during selection of Resources." );
    return QList<Resource>();
  }

  QList<Resource> resources;
  while ( query.next() ) {
    int id = query.value( 0 ).toInt();
    QString name = query.value( 1 ).toString();
    int id_res = query.value( 0 ).toInt();

    resources.append( Resource( id, name, id_res ) );
  }

  return resources;
}

QList<Resource> DataStore::listResources( const CachePolicy & policy )
{
  if ( !m_dbOpened )
    return QList<Resource>();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT id, name, cachepolicy_id FROM Resources WHERE cachepolicy_id = :id") );
  query.bindValue( QLatin1String(":id"), policy.id() );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of Resources by a Policy." );
    return QList<Resource>();
  }

  QList<Resource> resources;
  while ( query.next() ) {
    int id = query.value( 0 ).toInt();
    QString name = query.value( 1 ).toString();
    int id_res = query.value( 0 ).toInt();

    resources.append( Resource( id, name, id_res ) );
  }

  return resources;
}



bool Akonadi::DataStore::appendPersisntentSearch(const QString & name, const QByteArray & queryString)
{
  if ( !m_dbOpened ) return false;

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("INSERT INTO PersistentSearches (name, query) VALUES (:name, :query)") );
  query.bindValue( QLatin1String(":name"), name );
  query.bindValue( QLatin1String(":query"), queryString );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Unable to append persistent search" );
    return false;
  }

  mNotificationCollector->collectionAdded( name );
  return true;
}

bool Akonadi::DataStore::removePersistentSearch( const PersistentSearch &search )
{
  // TODO
//   mNotificationCollector->collectionRemoved( search.name() );
  return removeById( search.id(), QLatin1String("PersistentSearches") );
}

PersistentSearch Akonadi::DataStore::persistentSearch(const QString & name)
{
  if ( !m_dbOpened ) return PersistentSearch();

  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT id, name, query FROM PersistentSearches WHERE name = :name") );
  query.bindValue( QLatin1String(":name"), name );

  if ( !query.exec() || !query.next() ) {
    debugLastQueryError( query, "Error during selection of persistent search" );
    return PersistentSearch();
  }

  int id = query.value( 0 ).toInt();
  QString searchName = query.value( 1 ).toString();
  QByteArray queryString = query.value( 2 ).toByteArray();

  return PersistentSearch( id, searchName, queryString );
}

CollectionList Akonadi::DataStore::listPersistentSearches() const
{
  QSqlQuery query( m_database );
  query.prepare( QLatin1String("SELECT name FROM PersistentSearches") );
  if ( query.exec() ) {
    CollectionList list;
    while ( query.next() ) {
      QString name = query.value( 0 ).toString();
      list.append( QLatin1String("Search/") + name );
    }
    return list;
  }
  return CollectionList();
}



void DataStore::debugLastDbError( const char* actionDescription ) const
{
  qDebug() << actionDescription
           << "\nDriver said: "
           << m_database.lastError().driverText()
           << "\nDatabase said: "
           << m_database.lastError().databaseText();
}

void DataStore::debugLastQueryError( const QSqlQuery &query, const char* actionDescription ) const
{
  qDebug() << actionDescription
           << ": " << query.lastError().text();
}

bool DataStore::removeById( int id, const QString & tableName )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );
  const QString statement = QString::fromLatin1( "DELETE FROM %1 WHERE id = :id" ).arg( tableName );
  query.prepare( statement );
  query.bindValue( QLatin1String(":id"), id );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during deletion of a single row by ID from table %1: " + tableName.toLatin1() );
    return false;
  }

  return true;
}

Location DataStore::locationByName( const QString& name ) const
{
  qDebug() << "locationByName( " << name << " )";

  if ( !m_dbOpened )
    return Location();

  QSqlQuery query( m_database );
  const QString statement = QString::fromLatin1( "SELECT id, uri, cachepolicy_id, resource_id, exists_count, recent_count,"
                                     " unseen_count, first_unseen, uid_validity FROM Locations "
                                     "WHERE uri = :uri" );
  query.prepare( statement );
  query.bindValue( QLatin1String(":uri"), name );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during selection of a Location by name." );
    return Location();
  }

  Location location;
  while ( query.next() ) {
    location.fillFromQuery( query );
    location.setMimeTypes( mimeTypesForLocation( location.id() ) );
  }

  qDebug() << "location: " << location.isValid();

  return location;
}


int DataStore::uidNext() const
{
    // FIXME We can't use max(id) FROM PimItems because this is wrong if the
    //       entry with the highest id is deleted. Instead we should probably
    //       keep record of the largest id that any PimItem ever had.
    return highestPimItemId() + 1;
}


//static
int DataStore::lastInsertId( const QSqlQuery & query )
{
    QVariant v = query.lastInsertId();
    if ( !v.isValid() ) return -1;
    bool ok;
    int insertId = v.toInt( &ok );
    if ( !ok ) return -1;
    return insertId;
}


// static
QString DataStore::dateTimeFromQDateTime( const QDateTime & dateTime )
{
    QDateTime utcDateTime = dateTime;
    if ( utcDateTime.timeSpec() != Qt::UTC )
        utcDateTime.toUTC();
    return utcDateTime.toString( QLatin1String("yyyy-MM-dd hh:mm:ss") );
}


// static
QDateTime DataStore::dateTimeToQDateTime( const QByteArray & dateTime )
{
    return QDateTime::fromString( QString::fromLatin1(dateTime), QLatin1String("yyyy-MM-dd hh:mm:ss") );
}

bool Akonadi::DataStore::beginTransaction()
{
  qDebug() << "DataStore::beginTransaction()";
  if ( !m_dbOpened )
    return false;

  if ( m_inTransaction ) {
    qWarning() << "DataStore::beginTransaction(): Transaction already in progress!";
    return false;
  }

  QSqlDriver *driver = m_database.driver();
  if ( !driver->beginTransaction() ) {
    debugLastDbError( "DataStore::beginTransaction" );
    return false;
  }

  m_inTransaction = true;
  return true;
}

bool Akonadi::DataStore::rollbackTransaction()
{
  qDebug() << "DataStore::rollbackTransaction()";
  if ( !m_dbOpened )
    return false;

  if ( !m_inTransaction ) {
    qWarning() << "DataStore::rollbackTransaction(): No Transaction in progress!";
    return false;
  }

  QSqlDriver *driver = m_database.driver();
  m_inTransaction = false;
  emit transactionRolledBack();
  if ( !driver->rollbackTransaction() ) {
    debugLastDbError( "DataStore::rollbackTransaction" );
    return false;
  }

  return true;
}

bool Akonadi::DataStore::commitTransaction()
{
  qDebug() << "DataStore::commitTransaction()";
  if ( !m_dbOpened )
    return false;

  if ( !m_inTransaction ) {
    qWarning() << "DataStore::commitTransaction(): No Transaction in progress!";
    return false;
  }

  QSqlDriver *driver = m_database.driver();
  if ( !driver->commitTransaction() ) {
    debugLastDbError( "DataStore::commitTransaction" );
    rollbackTransaction();
    m_inTransaction = false;
    return false;
  }

  m_inTransaction = false;
  emit transactionCommitted();
  return true;
}

bool Akonadi::DataStore::inTransaction() const
{
  return m_inTransaction;
}

#include "datastore.moc"
