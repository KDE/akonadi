/***************************************************************************
 *   Copyright (C) 2006 by Andreas Gungl <a.gungl@gmx.de>                  *
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
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

#include "datastore.h"

#include "agentmanagerinterface.h"
#include "resourceinterface.h"
#include "dbinitializer.h"
#include "dbupdater.h"
#include "notificationmanager.h"
#include "tracer.h"
#include "selectquerybuilder.h"
#include "handlerhelper.h"
#include "countquerybuilder.h"
#include <akonadi/private/xdgbasedirs_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QSettings>
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

using namespace Akonadi;

QList<int> DataStore::mPendingItemDeliveries;
QMutex DataStore::mPendingItemDeliveriesMutex;
QWaitCondition DataStore::mPendingItemDeliveriesCondition;

QString DataStore::mDbDriverName;
QString DataStore::mDbName;
QString DataStore::mDbHostName;
QString DataStore::mDbUserName;
QString DataStore::mDbPassword;
QString DataStore::mDbConnectionOptions;

/***************************************************************************
 *   DataStore                                                           *
 ***************************************************************************/
DataStore::DataStore() :
  QObject(),
  m_dbOpened( false ),
  m_transactionLevel( 0 ),
  mNotificationCollector( new NotificationCollector( this ) )
{
  // load database settings if needed
  if ( mDbDriverName.isEmpty() ) {
    const QString serverConfigFile = XdgBaseDirs::akonadiServerConfigFile( XdgBaseDirs::ReadWrite );

    QSettings settings( serverConfigFile, QSettings::IniFormat );
    QString defaultDriver = QLatin1String("QMYSQL");

    mDbDriverName = settings.value( QLatin1String("General/Driver"), defaultDriver ).toString();

    // ensure we have a database driver
    if ( mDbDriverName.isEmpty() )
      mDbDriverName = defaultDriver;

    // useful defaults depending on the driver
    QString defaultDbName;
    QString defaultOptions;
    if ( mDbDriverName == QLatin1String("QMYSQL_EMBEDDED") ) {
      defaultDbName = QLatin1String( "akonadi" );
      defaultOptions = QString::fromLatin1( "SERVER_DATADIR=%1" ).arg( storagePath() );
    } else if ( mDbDriverName == QLatin1String( "QMYSQL" ) ) {
      defaultDbName = QLatin1String( "akonadi" );
      const QString miscDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi/db_misc" ) );
      defaultOptions = QString::fromLatin1( "UNIX_SOCKET=%1/mysql.socket" ).arg( miscDir );
    } else if ( mDbDriverName == QLatin1String( "QSQLITE" ) ) {
      defaultDbName = storagePath();
    }

    // read settings for current driver
    settings.beginGroup( mDbDriverName );
    mDbName = settings.value( QLatin1String( "Name" ), defaultDbName ).toString();
    mDbHostName = settings.value( QLatin1String( "Host" ) ).toString();
    mDbUserName = settings.value( QLatin1String( "User" ) ).toString();
    mDbPassword = settings.value( QLatin1String( "Password" ) ).toString();
    mDbConnectionOptions = settings.value( QLatin1String( "Options" ), defaultOptions ).toString();
    settings.endGroup();

    // store back the default values
    settings.setValue( QLatin1String( "General/Driver" ), mDbDriverName );
    settings.beginGroup( mDbDriverName );
    settings.setValue( QLatin1String( "Name" ), mDbName );
    settings.setValue( QLatin1String( "User" ), mDbUserName );
    settings.setValue( QLatin1String( "Password" ), mDbPassword );
    settings.setValue( QLatin1String( "Options" ), mDbConnectionOptions );
    settings.endGroup();
    settings.sync();
  }

  open();

  m_transactionLevel = 0;
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

  m_database = QSqlDatabase::addDatabase( driverName(), m_connectionName );
  if ( !mDbName.isEmpty() )
    m_database.setDatabaseName( mDbName );
  if ( !mDbHostName.isEmpty() )
    m_database.setHostName( mDbHostName );
  if ( !mDbUserName.isEmpty() )
    m_database.setUserName( mDbUserName );
  if ( !mDbPassword.isEmpty() )
    m_database.setPassword( mDbPassword );
  m_database.setConnectOptions( mDbConnectionOptions );

  if ( !m_database.isValid() ) {
    m_dbOpened = false;
    return;
  }
  m_dbOpened = m_database.open();

  if ( !m_dbOpened )
    debugLastDbError( "Cannot open database." );
  else
    qDebug() << "Database" << m_database.databaseName() << "opened using driver" << m_database.driverName();

  Q_ASSERT( m_database.driver()->hasFeature( QSqlDriver::LastInsertId ) );
  //Q_ASSERT( m_database.driver()->hasFeature( QSqlDriver::Transactions ) );
}

void Akonadi::DataStore::close()
{
  if ( !m_dbOpened )
    return;

  if ( inTransaction() ) {
    // By setting m_transactionLevel to '1' here, we skip all nested transactions
    // and rollback the outermost transaction.
    m_transactionLevel = 1;
    rollbackTransaction();
  }

  m_database.close();
  m_database = QSqlDatabase();
  QSqlDatabase::removeDatabase( m_connectionName );

  m_dbOpened = false;
}

bool Akonadi::DataStore::init()
{
  DbInitializer initializer( m_database, QLatin1String(":akonadidb.xml") );
  if (! initializer.run() ) {
    qWarning() << initializer.errorMsg();
    return false;
  }

  DbUpdater updater( m_database, QLatin1String( ":dbupdate.xml" ) );
  if ( !updater.run() )
    return false;

  // enable caching for some tables
  MimeType::enableCache( true );
  Flag::enableCache( true );
  Resource::enableCache( true );
  Location::enableCache( true );

  return true;
}

QThreadStorage<DataStore*> instances;

DataStore * Akonadi::DataStore::self()
{
  if ( !instances.hasLocalData() )
    instances.setLocalData( new DataStore() );
  return instances.localData();
}



QString DataStore::storagePath() const
{
  /**
   * We need the following path for the database directory:
   *   $XDG_DATA_HOME/akonadi/db/akonadi/
   */
  QString akonadiHomeDir = XdgBaseDirs::saveDir( "data", QLatin1String( "akonadi" ) );
  if ( akonadiHomeDir.isEmpty() ) {
    Tracer::self()->error( "DataStore::storagePath",
                           QString::fromLatin1( "Unable to create directory '%1/akonadi'" ).arg( XdgBaseDirs::homePath( "data" ) ) );
  }

  akonadiHomeDir += QLatin1Char( '/' );

  if ( driverName() == QLatin1String( "QSQLITE" ) ) {
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
  }
  if ( driverName() == QLatin1String( "QMYSQL_EMBEDDED" ) ) {
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
    return dbDataDir;
  }

  return QString();
}


/* --- Flag ---------------------------------------------------------- */
bool DataStore::appendFlag( const QString & name )
{
  if ( Flag::exists( name ) ) {
    qDebug() << "Cannot insert flag " << name
             << " because it already exists.";
    return false;
  }

  Flag flag( name );
  return flag.insert();
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

bool DataStore::appendItemFlags( const PimItem &item, const QList<Flag> &flags,
                                 bool checkIfExists, const Location &loc )
{
  if ( !item.isValid() )
    return false;
  if ( flags.isEmpty() )
    return true;

  for ( int i = 0; i < flags.count(); ++i ) {
    if ( !checkIfExists || !item.relatesToFlag( flags[ i ] ) ) {
      if ( !item.addFlag( flags[i] ) )
        return false;
    }
  }

  mNotificationCollector->itemChanged( item, loc );
  return true;
}

bool DataStore::appendItemFlags( const PimItem &item, const QList<QByteArray> &flags,
                                 bool checkIfExists, const Location &loc )
{
  Flag::List list;
  foreach ( const QByteArray f, flags ) {
    Flag flag = Flag::retrieveByName( QString::fromUtf8( f ) );
    if ( !flag.isValid() ) {
      flag = Flag( QString::fromUtf8( f ) );
      if ( !flag.insert() )
        return false;
    }
    list << flag;
  }
  return appendItemFlags( item, list, checkIfExists, loc );
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

/* --- ItemParts ----------------------------------------------------- */

bool DataStore::removeItemParts( const PimItem &item, const QList<QByteArray> &parts )
{
  Part::List existingParts = item.parts();
  foreach ( Part part, existingParts )
    if( parts.contains( part.name().toLatin1() ) )
      part.remove();

  mNotificationCollector->itemChanged( item );
  return true;
}

/* --- Location ------------------------------------------------------ */
bool DataStore::appendLocation( Location &location )
{
  // no need to check for already existing collection with the same name,
  // a unique index on parent + name prevents that in the database
  if ( !location.insert() )
    return false;

  mNotificationCollector->collectionAdded( location );
  return true;
}

bool Akonadi::DataStore::cleanupLocation(Location & location)
{
  // delete the content
  PimItem::List items = location.items();
  foreach ( PimItem item, items )
    cleanupPimItem( item );

  // delete location mimetypes
  removeMimeTypesForLocation( location.id() );
  Location::clearPimItems( location.id() );

  // delete attributes
  foreach ( LocationAttribute attr, location.attributes() )
    if ( !attr.remove() )
      return false;

  // delete the location itself
  mNotificationCollector->collectionRemoved( location );
  return location.remove();
}

bool Akonadi::DataStore::renameLocation( Location & location, qint64 newParent, const QByteArray & newName)
{
  if ( location.name() == newName && location.parentId() == newParent )
    return true;

  if ( !m_dbOpened )
    return false;

  if ( newParent > 0 ) {
    Location parent = Location::retrieveById( newParent );
    if ( !parent.isValid() )
      return false;
  }

  const bool move = location.parentId() != newParent;

  SelectQueryBuilder<Location> qb;
  qb.addValueCondition( Location::parentIdColumn(), Query::Equals, newParent );
  qb.addValueCondition( Location::nameColumn(), Query::Equals, newName );
  if ( !qb.exec() || qb.result().count() > 0 )
    return false;

  location.setName( newName );
  location.setParentId( newParent );

  if ( !location.update() )
    return false;

  mNotificationCollector->collectionChanged( location );
  return true;
}


bool DataStore::appendMimeTypeForLocation( qint64 locationId, const QStringList & mimeTypes )
{
  if ( mimeTypes.isEmpty() )
    return true;
  SelectQueryBuilder<MimeType> qb;
  qb.addValueCondition( MimeType::nameColumn(), Query::In, mimeTypes );
  if ( !qb.exec() )
    return false;
  QStringList missingMimeTypes = mimeTypes;

  foreach ( const MimeType mt, qb.result() ) {
    // unique index on n:m relation prevents duplicates, ie. this will fail
    // if this mimetype is already set
    if ( !Location::addMimeType( locationId, mt.id() ) )
      return false;
    missingMimeTypes.removeAll( mt.name() );
  }

  // the MIME type doesn't exist, so we have to add it to the db
  foreach ( const QString mtName, missingMimeTypes ) {
    qint64 mimeTypeId;
    if ( !appendMimeType( mtName, &mimeTypeId ) )
      return false;
    if ( !Location::addMimeType( locationId, mimeTypeId ) )
      return false;
  }

  return true;
}

bool Akonadi::DataStore::removeMimeTypesForLocation(qint64 locationId)
{
  return Location::clearMimeTypes( locationId );
}


void DataStore::activeCachePolicy(Location & loc)
{
  if ( !loc.cachePolicyInherit() )
    return;

  Location parent = loc.parent();
  while ( parent.isValid() ) {
    if ( !parent.cachePolicyInherit() ) {
      loc.setCachePolicyCheckInterval( parent.cachePolicyCheckInterval() );
      loc.setCachePolicyCacheTimeout( parent.cachePolicyCacheTimeout() );
      loc.setCachePolicySyncOnDemand( parent.cachePolicySyncOnDemand() );
      loc.setCachePolicyLocalParts( parent.cachePolicyLocalParts() );
      return;
    }
    parent = parent.parent();
  }

  // ### system default
  loc.setCachePolicyCheckInterval( -1 );
  loc.setCachePolicyCacheTimeout( -1 );
  loc.setCachePolicySyncOnDemand( false );
  loc.setCachePolicyLocalParts( QLatin1String("ALL") );
}

/* --- MimeType ------------------------------------------------------ */
bool DataStore::appendMimeType( const QString & mimetype, qint64 *insertId )
{
  if ( MimeType::exists( mimetype ) ) {
    qDebug() << "Cannot insert mimetype " << mimetype
             << " because it already exists.";
    return false;
  }

  MimeType mt( mimetype );
  return mt.insert( insertId );
}



/* --- PimItem ------------------------------------------------------- */
bool DataStore::appendPimItem( const QList<Part> & parts,
                               const MimeType & mimetype,
                               const Location & location,
                               const QDateTime & dateTime,
                               const QByteArray & remote_id,
                               PimItem &pimItem )
{
  pimItem.setMimeTypeId( mimetype.id() );
  pimItem.setLocationId( location.id() );
  if ( dateTime.isValid() )
    pimItem.setDatetime( dateTime );
  if ( remote_id.isEmpty() ) {
    // from application
    pimItem.setDirty( true );
  } else {
    // from resource
    pimItem.setRemoteId( remote_id );
    pimItem.setDirty( false );
  }
  pimItem.setAtime( QDateTime::currentDateTime() );

  if ( !pimItem.insert() )
    return false;

  // insert every part
  if ( !parts.isEmpty() ) {
    foreach( Part part, parts ) {
      part.setPimItemId( pimItem.id() );
      part.setDatasize( part.data().size() );

      if( !part.insert() )
        return false;
    }
  }

  mNotificationCollector->itemAdded( pimItem, location, mimetype.name() );
  return true;
}

bool Akonadi::DataStore::updatePimItem(PimItem & pimItem)
{
  pimItem.setAtime( QDateTime::currentDateTime() );
  if ( mSessionId != pimItem.location().resource().name().toLatin1() )
    pimItem.setDirty( true );
  if ( !pimItem.update() )
    return false;

  mNotificationCollector->itemChanged( pimItem );
  return true;
}

bool Akonadi::DataStore::updatePimItem(PimItem & pimItem, const Location & destination)
{
  if ( !pimItem.isValid() || !destination.isValid() )
    return false;
  if ( pimItem.locationId() == destination.id() )
    return true;

  Location source = pimItem.location();
  if ( !source.isValid() )
    return false;
  mNotificationCollector->collectionChanged( source );

  pimItem.setLocationId( destination.id() );
  pimItem.setAtime( QDateTime::currentDateTime() );
  if ( mSessionId != pimItem.location().resource().name().toLatin1() )
    pimItem.setDirty( true );
  if ( !pimItem.update() )
    return false;

  mNotificationCollector->collectionChanged( destination );
  mNotificationCollector->itemMoved( pimItem, source, destination );
  return true;
}

bool DataStore::updatePimItem(PimItem & pimItem, const QString & remoteId)
{
  if ( !pimItem.isValid() )
    return false;
  if ( pimItem.remoteId() == remoteId.toLatin1() )
    return true;

  pimItem.setRemoteId( remoteId.toLatin1() );
  pimItem.setAtime( QDateTime::currentDateTime() );
  if ( !pimItem.update() )
    return false;
  mNotificationCollector->itemChanged( pimItem );
  return true;
}



bool DataStore::cleanupPimItem( const PimItem &item )
{
  if ( !item.isValid() )
    return false;

  // generate the notification before actually removing the data
  mNotificationCollector->itemRemoved( item );

  if ( !item.clearFlags() )
    return false;
  if ( !Part::remove( Part::pimItemIdColumn(), item.id() ) )
    return false;
  if ( !PimItem::remove( PimItem::idColumn(), item.id() ) )
    return false;

  if ( !Entity::clearRelation<LocationPimItemRelation>( item.id(), Entity::Right ) )
    return false;

  return true;
}

bool DataStore::cleanupPimItems( const Location &location )
{
  if ( !m_dbOpened || !location.isValid() )
    return false;

  QueryBuilder qb( QueryBuilder::Select );
  qb.addTable( Flag::tableName() );
  qb.addTable( PimItemFlagRelation::tableName() );
  qb.addTable( PimItem::tableName() );
  qb.addColumn( PimItemFlagRelation::leftFullColumnName() );
  qb.addValueCondition( Flag::nameFullColumnName(), Query::Equals, QLatin1String("\\Deleted") );
  qb.addValueCondition( PimItem::locationIdFullColumnName(), Query::Equals, location.id() );
  qb.addColumnCondition( PimItemFlagRelation::rightFullColumnName(), Query::Equals, Flag::idFullColumnName() );

  if ( !qb.exec() )
    return false;

  QList<PimItem> pimItems;
  while ( qb.query().next() ) {
    PimItem item;
    item.setId( qb.query().value( 0 ).toLongLong() );

    pimItems.append( item );
  }

  bool ok = true;
  for ( int i = 0; i < pimItems.count(); ++i )
    ok = ok && cleanupPimItem( pimItems[ i ] );

  return ok;
}

void Akonadi::DataStore::retrieveDataFromResource( qint64 uid, const QByteArray& remote_id,
                                                   const QString &resource, const QStringList &parts )
{
  // TODO: error handling
  qDebug() << "retrieveDataFromResource()" << uid;

  // check if that item is already being fetched by someone else
  mPendingItemDeliveriesMutex.lock();
  if ( mPendingItemDeliveries.contains( uid ) ) {
      qDebug() << "requestItemDelivery(): item already requested by other thread - waiting" << uid;
      mPendingItemDeliveriesCondition.wait( &mPendingItemDeliveriesMutex );
      qDebug() << "requestItemDelivery(): continuing";
      forever {
        if ( !mPendingItemDeliveries.contains( uid ) ) {
          mPendingItemDeliveriesMutex.unlock();
          break;
        }
        qDebug() << "requestItemDelivery(): item still requested by other thread - waiting again" << uid;
        mPendingItemDeliveriesCondition.wait( &mPendingItemDeliveriesMutex );
      }
  } else {
      qDebug() << "requestItemDelivery(): blocking uid" << uid;
      mPendingItemDeliveries << uid;
      mPendingItemDeliveriesMutex.unlock();

      qDebug() << "requestItemDelivery(): requested parts:" << parts;

      // call the resource
      org::kde::Akonadi::Resource *interface = resourceInterface( resource );
      if ( interface )
        interface->requestItemDelivery( uid, QString::fromUtf8(remote_id), parts );

      mPendingItemDeliveriesMutex.lock();
      qDebug() << "requestItemDelivery(): freeing uid" << uid;
      mPendingItemDeliveries.removeAll( uid );
      mPendingItemDeliveriesCondition.wakeAll();
      mPendingItemDeliveriesMutex.unlock();
  }
}

void DataStore::triggerCollectionSync( const Location &location )
{
  org::kde::Akonadi::Resource *interface = resourceInterface( location.resource().name() );
  if ( interface )
    interface->synchronizeCollection( location.id() );
}

QList<PimItem> DataStore::listPimItems( const Location & location, const Flag &flag )
{
  if ( !m_dbOpened )
    return QList<PimItem>();

  SelectQueryBuilder<PimItem> qb;
  qb.addTable( PimItemFlagRelation::tableName() );
  qb.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, PimItemFlagRelation::leftFullColumnName() );
  qb.addValueCondition( PimItemFlagRelation::rightFullColumnName(), Query::Equals, flag.id() );

  if ( location.isValid() )
    qb.addValueCondition( PimItem::locationIdFullColumnName(), Query::Equals, location.id() );

   if ( !qb.exec() )
    return QList<PimItem>();

  return qb.result();
}

qint64 DataStore::highestPimItemId() const
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

  return query.value( 0 ).toLongLong();
}


bool DataStore::addCollectionAttribute(const Location & loc, const QByteArray & key, const QByteArray & value)
{
  SelectQueryBuilder<LocationAttribute> qb;
  qb.addValueCondition( LocationAttribute::locationIdColumn(), Query::Equals, loc.id() );
  qb.addValueCondition( LocationAttribute::typeColumn(), Query::Equals, key );
  if ( !qb.exec() )
    return false;

  if ( qb.result().count() > 0 ) {
    qDebug() << "Attribute" << key << "already exists for location" << loc.id();
    return false;
  }

  LocationAttribute attr;
  attr.setLocationId( loc.id() );
  attr.setType( key );
  attr.setValue( value );

  if ( !attr.insert() )
    return false;

  mNotificationCollector->collectionChanged( loc );
  return true;
}

bool Akonadi::DataStore::removeCollectionAttribute(const Location & loc, const QByteArray & key)
{
  SelectQueryBuilder<LocationAttribute> qb;
  qb.addValueCondition( LocationAttribute::locationIdColumn(), Query::Equals, loc.id() );
  qb.addValueCondition( LocationAttribute::typeColumn(), Query::Equals, key );
  if ( !qb.exec() )
    return false;

  foreach ( LocationAttribute attr, qb.result() ) {
    if ( !attr.remove() )
      return false;
  }

  mNotificationCollector->collectionChanged( loc );
  return true;
}


void DataStore::debugLastDbError( const char* actionDescription ) const
{
  Tracer::self()->error( "DataStore (Database Error)",
                         QString::fromLatin1( "%1\nDriver said: %2\nDatabase said:%3" )
                            .arg( QString::fromLatin1( actionDescription ) )
                            .arg( m_database.lastError().driverText() )
                            .arg( m_database.lastError().databaseText() )
                       );
}

void DataStore::debugLastQueryError( const QSqlQuery &query, const char* actionDescription ) const
{
  Tracer::self()->error( "DataStore (Database Query Error)",
                         QString::fromLatin1( "%1: %2" )
                            .arg( QString::fromLatin1( actionDescription ) )
                            .arg( query.lastError().text() )
                       );
}

qint64 DataStore::uidNext() const
{
    // FIXME We can't use max(id) FROM PimItems because this is wrong if the
    //       entry with the highest id is deleted. Instead we should probably
    //       keep record of the largest id that any PimItem ever had.
    return highestPimItemId() + 1;
}


//static
qint64 DataStore::lastInsertId( const QSqlQuery & query )
{
    QVariant v = query.lastInsertId();
    if ( !v.isValid() ) return -1;
    bool ok;
    qint64 insertId = v.toLongLong( &ok );
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
  if ( !m_dbOpened )
    return false;

  if ( m_transactionLevel == 0 ) {
    QSqlDriver *driver = m_database.driver();
    if ( !driver->beginTransaction() ) {
      debugLastDbError( "DataStore::beginTransaction" );
      return false;
    }
  }

  m_transactionLevel++;

  return true;
}

bool Akonadi::DataStore::rollbackTransaction()
{
  if ( !m_dbOpened )
    return false;

  if ( m_transactionLevel == 0 ) {
    qWarning() << "DataStore::rollbackTransaction(): No transaction in progress!";
    return false;
  }

  m_transactionLevel--;

  if ( m_transactionLevel == 0 ) {
    QSqlDriver *driver = m_database.driver();
    emit transactionRolledBack();
    if ( !driver->rollbackTransaction() ) {
      debugLastDbError( "DataStore::rollbackTransaction" );
      return false;
    }
  }

  return true;
}

bool Akonadi::DataStore::commitTransaction()
{
  if ( !m_dbOpened )
    return false;

  if ( m_transactionLevel == 0 ) {
    qWarning() << "DataStore::commitTransaction(): No transaction in progress!";
    return false;
  }

  if ( m_transactionLevel == 1 ) {
    QSqlDriver *driver = m_database.driver();
    if ( !driver->commitTransaction() ) {
      debugLastDbError( "DataStore::commitTransaction" );
      rollbackTransaction();
      return false;
    } else {
      emit transactionCommitted();
    }
  }

  m_transactionLevel--;
  return true;
}

bool Akonadi::DataStore::inTransaction() const
{
  return m_transactionLevel > 0;
}

org::kde::Akonadi::Resource * Akonadi::DataStore::resourceInterface( const QString &res )
{
  org::kde::Akonadi::Resource* iface = 0;
  if ( mResourceInterfaceCache.contains( res ) )
    iface = mResourceInterfaceCache.value( res );
  if ( iface && iface->isValid() )
    return iface;

  delete iface;
  iface = new org::kde::Akonadi::Resource( QLatin1String("org.kde.Akonadi.Resource.") + res,
                                           QLatin1String("/"), QDBusConnection::sessionBus(), this );
  if ( !iface || !iface->isValid() ) {
    qDebug() << QString::fromLatin1( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                                    .arg( res, iface ? iface->lastError().message() : QString() );
    delete iface;
    return 0;
  }
  mResourceInterfaceCache.insert( res, iface );
  return iface;
}

#include "datastore.moc"
