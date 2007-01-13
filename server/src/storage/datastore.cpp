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
#include <QtCore/QThreadStorage>
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

QThreadStorage<DataStore*> instances;

DataStore * Akonadi::DataStore::self()
{
  if ( !instances.hasLocalData() )
    instances.setLocalData( new DataStore() );
  return instances.localData();
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
    resource = Resource::retrieveByName( resourceName );
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

  qDebug() << "Resource: " << resource.name() << " fullPrefix: " << fullPrefix << " pattern: " << sanitizedPattern;

  if ( !fullPrefix.isEmpty() ) {
    result.setValid( false );
  }

  const QList<Location> locations = listLocations( resource );
  foreach( Location l, locations ) {
    const QString location = locationDelimiter() + l.name();
#if 0
    qDebug() << "Location: " << location << " l: " << l << " prefix: " << fullPrefix;
#endif
    const bool atFirstLevel =
      location.lastIndexOf( locationDelimiter() ) == fullPrefix.lastIndexOf( locationDelimiter() );
    if ( location.startsWith( fullPrefix ) ) {
      if ( hasStar || ( hasPercent && atFirstLevel ) ||
           location == fullPrefix + sanitizedPattern ) {
        Collection c( location.right( location.size() -1 ) );
        c.setMimeTypes( MimeType::joinByName<MimeType>( l.mimeTypes(), QLatin1String(",") ).toLatin1() );
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
  if ( Flag::exists( name ) ) {
    qDebug() << "Cannot insert flag " << name
             << " because it already exists.";
    return false;
  }

  return Flag::insert( Flag( name ) );
}

bool DataStore::removeFlag( const Flag & flag )
{
  return removeFlag( flag.id() );
}

bool DataStore::removeFlag( int id )
{
  return removeById( id, Flag::tableName() );
}

/* --- ItemFlags ----------------------------------------------------- */

bool DataStore::setItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  // first delete all old flags of this pim item
  if ( !item.clearFlags() )
    return false;

  // then add the new flags
  for ( int i = 0; i < flags.count(); ++i ) {
    if ( !item.addFlag( flags[i] ) )
      return false;
  }

  mNotificationCollector->itemChanged( item );
  return true;
}

bool DataStore::appendItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  for ( int i = 0; i < flags.count(); ++i ) {
    if ( !item.relatesToFlag( flags[ i ] ) ) {
      if ( !item.addFlag( flags[i] ) )
        return false;
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
  for ( int i = 0; i < flags.count(); ++i ) {
    if ( !item.removeFlag( flags[ i ] ) )
      return false;
  }

  mNotificationCollector->itemChanged( item );
  return true;
}


/* --- CachePolicy --------------------------------------------------- */
bool DataStore::appendCachePolicy( const QString & policy )
{
  if ( CachePolicy::exists( policy ) ) {
    qDebug() << "Cannot insert policy " << policy
             << " because it already exists.";
    return false;
  }
  return CachePolicy::insert( CachePolicy( policy ) );
}

bool DataStore::removeCachePolicy( const CachePolicy & policy )
{
  return removeCachePolicy( policy.id() );
}

bool DataStore::removeCachePolicy( int id )
{
  return removeById( id, CachePolicy::tableName() );
}


/* --- Location ------------------------------------------------------ */
bool DataStore::appendLocation( const QString & location,
                                const Resource & resource,
                                int *insertId )
{
  if ( Location::exists( location ) ) {
    qDebug() << "Cannot insert location " << location
             << " because it already exists.";
    return false;
  }

  Location loc;
  loc.setName( location );
  loc.setResourceId( resource.id() );
  loc.setExistCount( 0 );
  loc.setRecentCount( 0 );
  loc.setUnseenCount( 0 );
  loc.setFirstUnseen( 0 );
  loc.setUidValidity( 0 );
  if ( !Location::insert( loc, insertId ) )
    return false;

  mNotificationCollector->collectionAdded( location, resource.name().toLatin1() );
  return true;
}

bool DataStore::appendLocation( const QString & location,
                                const Resource & resource,
                                const CachePolicy & policy )
{
  if ( Location::exists( location ) ) {
    qDebug() << "Cannot insert location " << location
        << " because it already exists.";
    return false;
  }
  if ( !Location::insert( Location( location, policy.id(), resource.id(), 0, 0, 0, 0 ,0 ) ) )
    return false;

  mNotificationCollector->collectionAdded( location, resource.name().toLatin1() );
  return true;
}

bool DataStore::removeLocation( const Location & location )
{
  mNotificationCollector->collectionRemoved( location );
  return removeById( location.id(), Location::tableName() );
}

bool DataStore::removeLocation( int id )
{
  return removeLocation( Location::retrieveById( id ) );
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
        addToUpdateAssignments( assignments, existsChange, Location::existCountColumn() );
        addToUpdateAssignments( assignments, recentChange, Location::recentCountColumn() );
        addToUpdateAssignments( assignments, unseenChange, Location::unseenCountColumn() );
        QString q = QString::fromLatin1( "UPDATE %1 SET %2 WHERE id = :id" )
                    .arg( Location::tableName(), assignments.join(QLatin1String(",")) );
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
  return location.updateColumn( Location::cachePolicyIdColumn(), policy.id() );
}

bool DataStore::resetLocationPolicy( const Location & location )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  query.prepare( QString::fromLatin1("UPDATE %1 SET %2 = NULL WHERE %3 = :id")
      .arg( Location::tableName(), Location::cachePolicyIdColumn(), Location::idColumn() ) );
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
  query.prepare( QString::fromLatin1("UPDATE %1 SET %2 = :name WHERE %3 = :id")
      .arg( Location::tableName(), Location::nameColumn(), Location::idColumn() ) );
  query.bindValue( QLatin1String(":id"), location.id() );
  query.bindValue( QLatin1String(":name"), newName );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during renaming of a single location." );
    return false;
  }

  mNotificationCollector->collectionAdded( newName );
  return true;
}


bool DataStore::appendMimeTypeForLocation( int locationId, const QString & mimeType )
{
  //qDebug() << "DataStore::appendMimeTypeForLocation( " << locationId << ", '" << mimeType << "' )";
  int mimeTypeId;
  MimeType m = MimeType::retrieveByName( mimeType );
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
  if ( Location::relatesToMimeType( locationId, mimeTypeId ) ) {
    qDebug() << "Cannot insert location-mime type ( " << locationId
             << ", " << mimeTypeId << " ) because it already exists.";
    return false;
  }

  return Location::addMimeType( locationId, mimeTypeId );
}


bool Akonadi::DataStore::removeMimeTypesForLocation(int locationId)
{
  return Location::clearMimeTypes( locationId );
}


QList<Location> DataStore::listLocations( const Resource & resource ) const
{
  if ( resource.isValid() )
    return Location::retrieveFiltered( Location::resourceIdColumn(), resource.id() );
  return Location::retrieveAll();
}

/* --- MimeType ------------------------------------------------------ */
bool DataStore::appendMimeType( const QString & mimetype, int *insertId )
{
  if ( MimeType::exists( mimetype ) ) {
    qDebug() << "Cannot insert mimetype " << mimetype
             << " because it already exists.";
    return false;
  }

  return MimeType::insert( MimeType( mimetype ), insertId );
}

bool DataStore::removeMimeType( const MimeType & mimetype )
{
  return removeMimeType( mimetype.id() );
}

bool DataStore::removeMimeType( int id )
{
  return removeById( id, MimeType::tableName() );
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

  QStringList columns;
  columns << PimItem::remoteIdColumn() << PimItem::dataColumn() << PimItem::locationIdColumn() << PimItem::mimeTypeIdColumn();
  QString values = QLatin1String( ":remote_id, :data, :location_id, :mimetype_id");
  if ( dateTime.isValid() ) {
    columns << PimItem::datetimeColumn();
    values += QLatin1String( ", :datetime" );
  }
  QString statement = QString::fromLatin1( "INSERT INTO %1 (%2) VALUES (%3)" )
      .arg( PimItem::tableName(), columns.join( QLatin1String(",") ), values );

  QSqlQuery query( m_database );
  query.prepare( statement );
  query.bindValue( QLatin1String(":remote_id"), QString::fromUtf8(remote_id) );
  query.bindValue( QLatin1String(":data"), data );
  query.bindValue( QLatin1String(":location_id"), location.id() );
  query.bindValue( QLatin1String(":mimetype_id"), mimetype.id() );
  if ( dateTime.isValid() )
    query.bindValue( QLatin1String(":datetime"), dateTimeFromQDateTime( dateTime ) );

  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single PimItem." );
    return false;
  }

  int id = lastInsertId( query );
  if ( insertId )
      *insertId = id;

  mNotificationCollector->itemAdded( pimItemById( id ), location, mimetype.name().toLatin1() );
  return true;
}

bool Akonadi::DataStore::updatePimItem(const PimItem & pimItem, const QByteArray & data)
{
  if ( !pimItem.updateColumn( PimItem::dataColumn(), data ) )
    return false;

  mNotificationCollector->itemChanged( pimItem );
  return true;
}

bool Akonadi::DataStore::updatePimItem(const PimItem & pimItem, const Location & destination)
{
  if ( !pimItem.isValid() || !destination.isValid() )
    return false;

  Location source = pimItem.location();
  if ( !source.isValid() )
    return false;
  mNotificationCollector->collectionChanged( source );

  if ( !pimItem.updateColumn( PimItem::locationIdColumn(), destination.id() ) )
    return false;

  mNotificationCollector->collectionChanged( destination );
  return true;
}

bool DataStore::cleanupPimItem( const PimItem &item )
{
  if ( !item.isValid() )
    return false;

  // generate the notification before actually removing the data
  mNotificationCollector->itemRemoved( item );

  if ( !ItemMetaData::remove( ItemMetaData::pimItemIdColumn(), item.id() ) )
    return false;
  if ( !item.clearFlags() )
    return false;
  if ( !Part::remove( Part::pimItemIdColumn(), item.id() ) )
    return false;
  if ( !PimItem::remove( PimItem::idColumn(), item.id() ) )
    return false;

  return true;
}

bool DataStore::cleanupPimItems( const Location &location )
{
  if ( !m_dbOpened || !location.isValid() )
    return false;

  QStringList tables;
  tables << Flag::tableName() << PimItemFlagRelation::tableName() << PimItem::tableName();
  QString statement = QString::fromLatin1( "SELECT %1 FROM %2 " )
      .arg( PimItemFlagRelation::leftFullColumnName(), tables.join( QLatin1String(",") ) );
  statement += QString::fromLatin1( "WHERE %1 = '\\Deleted' AND " )
      .arg( Flag::nameFullColumnName() );
  statement += QString::fromLatin1( "%1 = %2 AND " )
      .arg( PimItemFlagRelation::rightFullColumnName(), Flag::idFullColumnName() );
  statement += QString::fromLatin1( "%1 = %2 AND " )
      .arg( PimItemFlagRelation::leftFullColumnName(), PimItem::idFullColumnName() );
  statement += QString::fromLatin1( "%1 = :locationId" )
      .arg( PimItem::locationIdFullColumnName() );

  QSqlQuery query( m_database );
  query.prepare( statement );
  query.bindValue( QLatin1String(":locationId"), location.id() );
  if ( !query.exec() ) {
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

  const QString statement = QString::fromLatin1( "SELECT %1 FROM %2 WHERE %3 = :locationId" )
      .arg( PimItem::idColumn(), PimItem::tableName(), PimItem::locationIdColumn() );

  QSqlQuery query( m_database );
  query.prepare( statement );
  query.bindValue( QLatin1String(":locationId"), item.locationId() );
  if ( !query.exec() ) {
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

QByteArray Akonadi::DataStore::retrieveDataFromResource( int uid, const QByteArray& remote_id,
                                                         int locationid, FetchQuery::Type type )
{
  // TODO: error handling
  qDebug() << "retrieveDataFromResource()" << uid;
  Location l = Location::retrieveById( locationid );
  Resource r = l.resource();

  // call the resource

  // Use the interface if we are in main thread, the DBusThread proxy otherwise
  if ( QThread::currentThread() == QCoreApplication::instance()->thread() ) {
      org::kde::Akonadi::Resource *interface =
                  new org::kde::Akonadi::Resource( QLatin1String("org.kde.Akonadi.Resource.") + r.name(),
                                                   QLatin1String("/"), QDBusConnection::sessionBus(), this );

      if ( !interface || !interface->isValid() ) {
         qDebug() << QString::fromLatin1( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                                        .arg( r.name(), interface ? interface->lastError().message() : QString() );
         return QByteArray();
      }
      bool ok = interface->requestItemDelivery( uid, QString::fromUtf8(remote_id), l.name(), type );
  } else {
    QList<QVariant> arguments;
    arguments << uid << QString::fromUtf8( remote_id ) << l.name() << type;

    DBusThread *dbusThread = static_cast<DBusThread*>( QThread::currentThread() );

    const QList<QVariant> result = dbusThread->callDBus( QLatin1String( "org.kde.Akonadi.Resource." ) + r.name(),
                                                         QLatin1String( "/" ),
                                                         QLatin1String( "org.kde.Akonadi.Resource" ),
                                                         QLatin1String( "requestItemDelivery" ), arguments );

    // do something with result...
    qDebug() << "got dbus response: " << result;
  }
  qDebug() << "returned from requestItemDelivery()";

  // get the delivered item
  QSqlQuery query( m_database );
  query.prepare( QString::fromLatin1("SELECT data FROM %1 WHERE %2 = :id")
      .arg( PimItem::tableName(), PimItem::idColumn() ) );
  query.bindValue( QLatin1String(":id"), uid );
  if ( query.exec() && query.next() )
    return query.value( 0 ).toByteArray();

  return QByteArray();
}


PimItem Akonadi::DataStore::pimItemById( int id, FetchQuery::Type type )
{
  if ( !m_dbOpened )
    return PimItem();

  const QString field = fieldNameForDataType( type );
  QStringList cols;
  cols << PimItem::idColumn() << PimItem::locationIdColumn() << PimItem::mimeTypeIdColumn()
       << PimItem::datetimeColumn() << PimItem::remoteIdColumn();
  QString statement = QString::fromLatin1( "SELECT %1, %2 FROM %3 WHERE %4 = :id" )
      .arg( cols.join( QLatin1String(",") ), field, PimItem::tableName(), PimItem::idColumn() );

  QSqlQuery query( m_database );
  query.prepare( statement );
  query.bindValue( QLatin1String(":id"), id );

  if ( !query.exec() || !query.next() ) {
    debugLastQueryError( query, "Error during selection of single PimItem." );
    return PimItem();
  }

  int pimItemId = query.value( 0 ).toInt();
  int location = query.value( 1 ).toInt();
  int mimetype = query.value( 2 ).toInt();
  QByteArray remote_id = query.value( 4 ).toByteArray();
  QDateTime dateTime = dateTimeToQDateTime( query.value( 3 ).toByteArray() );
  QByteArray data = query.value( 5 ).toByteArray();
  if ( data.isEmpty() && type == FetchQuery::AllType )
      data = retrieveDataFromResource( id, remote_id, location, type );

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

  QStringList cols;
  cols << PimItem::idFullColumnName() << PimItem::dataFullColumnName()
      << PimItem::mimeTypeIdFullColumnName() << PimItem::datetimeFullColumnName()
      << PimItem::remoteIdFullColumnName() << PimItem::locationIdFullColumnName();
  QString statement = QString::fromLatin1( "SELECT %1 FROM %2, %3 WHERE " )
      .arg( cols.join( QLatin1String(",") ), PimItem::tableName(), PimItemFlagRelation::tableName() );
  statement += QString::fromLatin1( "%1 = %2 AND %3 = :flag_id" )
      .arg( PimItemFlagRelation::leftFullColumnName(), PimItem::idFullColumnName(), PimItemFlagRelation::rightFullColumnName() );

  if ( location.isValid() )
    statement += QString::fromLatin1( " AND %1 = :location_id" )
        .arg( PimItem::locationIdFullColumnName() );

  QSqlQuery query( m_database );
  query.prepare( statement );
  query.bindValue( QLatin1String(":flag_id"), flag.id() );
  if ( location.isValid() )
    query.bindValue( QLatin1String(":location_id"), location.id() );

  if ( !query.exec() ) {
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
  const QString statement = QString::fromLatin1( "SELECT MAX(%1) FROM %2" ).arg( PimItem::idColumn(), PimItem::tableName() );

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
  if ( !location.isValid() )
    return -1;

  int cnt = PimItem::count( PimItem::locationIdColumn(), location.id() );
  if ( cnt < 0 )
    return -1;

  return cnt + 1;
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

  QString statement = QString::fromLatin1( "SELECT id FROM %1 WHERE (%2)" )
      .arg( PimItem::tableName(), statementParts.join( QLatin1String(" OR ") ) );
  if ( location.isValid() )
     statement += QString::fromLatin1( " AND %1 = :location_id" ).arg( PimItem::locationIdColumn() );

  QSqlQuery query( m_database );
  query.prepare( statement );
  if ( location.isValid() )
    query.bindValue( QLatin1String(":location_id"), location.id() );

  if ( !query.exec() ) {
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
      QString::fromLatin1( "SELECT %1 FROM %2 WHERE %3 = :id" ).arg( PimItem::idColumn(), PimItem::tableName(), PimItem::locationIdColumn() );
  query.prepare( statement );
  query.bindValue( QLatin1String(":id"), location.id() );

  if ( !query.exec() ) {
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
  if ( Resource::exists( resource ) ) {
    qDebug() << "Cannot insert resource " << resource
             << " because it already exists.";
    return false;
  }

  return Resource::insert( Resource( resource, policy.id() ) );
}

bool DataStore::removeResource( const Resource & resource )
{
  return removeResource( resource.id() );
}

bool DataStore::removeResource( int id )
{
  return removeById( id, Resource::tableName() );
}


QList<Resource> DataStore::listResources( const CachePolicy & policy )
{
  return Resource::retrieveFiltered( Resource::cachePolicyIdColumn(), policy.id() );
}

CollectionList Akonadi::DataStore::listPersistentSearches() const
{
  QList<PersistentSearch> list = PersistentSearch::retrieveAll();
  CollectionList rv;
  foreach ( PersistentSearch search, list )
    rv.append( QLatin1String("Search/") + search.name() );
  return rv;
}



bool Akonadi::DataStore::appendPersisntentSearch(const QString & name, const QByteArray & queryString)
{
  PersistentSearch::insert( PersistentSearch( name, queryString ) );
  mNotificationCollector->collectionAdded( name );
  return true;
}

bool Akonadi::DataStore::removePersistentSearch( const PersistentSearch &search )
{
  // TODO
//   mNotificationCollector->collectionRemoved( search.name() );
  return removeById( search.id(), PersistentSearch::tableName() );
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
