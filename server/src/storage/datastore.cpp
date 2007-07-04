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

#include "datastore.h"

#include "agentmanagerinterface.h"
#include "resourceinterface.h"
#include "dbinitializer.h"
#include "notificationmanager.h"
#include "tracer.h"
#include "selectquerybuilder.h"
#include "handlerhelper.h"
#include "countquerybuilder.h"

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
  m_inTransaction( false ),
  mNotificationCollector( new NotificationCollector( this ) )
{
  // load database settings if needed
  if ( mDbDriverName.isEmpty() ) {
    QSettings settings( QDir::homePath() + QLatin1String("/.akonadi/akonadiserverrc"), QSettings::IniFormat );
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
      defaultOptions = QString::fromLatin1( "UNIX_SOCKET=%1/db_misc/mysql.socket" )
                                     .arg( QDir::homePath() + QLatin1String( "/.akonadi/"  ) );
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
  if ( inTransaction() )
    rollbackTransaction();

  m_database.close();
  m_database = QSqlDatabase();
  QSqlDatabase::removeDatabase( m_connectionName );
}

bool Akonadi::DataStore::init()
{
  DbInitializer initializer( m_database, QLatin1String(":akonadidb.xml") );
  if (! initializer.run() ) {
    qWarning() << initializer.errorMsg();
    return false;
  }
  return true;
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


/* --- Location ------------------------------------------------------ */
bool DataStore::appendLocation( Location &location )
{
  SelectQueryBuilder<Location> qb;
  qb.addValueCondition( Location::parentIdColumn(), "=", location.parentId() );
  qb.addValueCondition( Location::nameColumn(), "=", location.name() );
  if ( !qb.exec() ) {
    qDebug() << "Unable to check location existence";
    return false;
  }
  if ( !qb.result().isEmpty() ) {
    qDebug() << "Cannot insert location " << location.name()
             << " because it already exists.";
    return false;
  }

  location.setExistCount( 0 );
  location.setRecentCount( 0 );
  location.setUnseenCount( 0 );
  location.setFirstUnseen( 0 );
  location.setUidValidity( 0 );
  if ( !location.insert() )
    return false;

  mNotificationCollector->collectionAdded( location );
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
  Location::clearPimItems( location.id() );

  // delete attributes
  foreach ( LocationAttribute attr, location.attributes() )
    if ( !attr.remove() )
      return false;

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

bool DataStore::changeLocationPolicy( Location & location,
                                      const CachePolicy & policy )
{
  location.setCachePolicyId( policy.id() );
  mNotificationCollector->collectionChanged( location );
  return location.update();
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

bool Akonadi::DataStore::renameLocation(const Location & location, int newParent, const QString & newName)
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
  qb.addValueCondition( Location::parentIdColumn(), "=", newParent );
  qb.addValueCondition( Location::nameColumn(), "=", newName );
  if ( !qb.exec() || qb.result().count() > 0 )
    return false;

  Location renamedLoc = location;

  renamedLoc.setName( newName );
  renamedLoc.setParentId( newParent );

  if ( !renamedLoc.update() )
    return false;

  mNotificationCollector->collectionChanged( renamedLoc );
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

CachePolicy DataStore::activeCachePolicy(const Location & loc)
{
  CachePolicy policy;
  policy = loc.cachePolicy();
  if ( policy.isValid() )
    return policy;

  Location parent = loc.parent();
  while ( parent.isValid() ) {
    policy = parent.cachePolicy();
    if ( policy.isValid() )
      return policy;
    parent = parent.parent();
  }

  // fall back to resource cache policy
  Resource res = loc.resource();
  return res.cachePolicy();
}

/* --- MimeType ------------------------------------------------------ */
bool DataStore::appendMimeType( const QString & mimetype, int *insertId )
{
  if ( MimeType::exists( mimetype ) ) {
    qDebug() << "Cannot insert mimetype " << mimetype
             << " because it already exists.";
    return false;
  }

  MimeType mt( mimetype );
  return mt.insert( insertId );
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
  PimItem pimItem;
  pimItem.setData( data );
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

  if ( !pimItem.insert( insertId ) )
    return false;

  mNotificationCollector->itemAdded( pimItem, location, mimetype.name() );
  return true;
}

bool Akonadi::DataStore::updatePimItem(PimItem & pimItem, const QByteArray & data)
{
  pimItem.setData( data );
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

  // check if that item is already been fetched by someone else
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

      // call the resource
      org::kde::Akonadi::Resource *interface =
            new org::kde::Akonadi::Resource( QLatin1String("org.kde.Akonadi.Resource.") + r.name(),
                                             QLatin1String("/"), QDBusConnection::sessionBus(), this );

      bool ok = false;
      if ( !interface || !interface->isValid() ) {
        qDebug() << QString::fromLatin1( "Cannot connect to agent instance with identifier '%1', error message: '%2'" )
                                        .arg( r.name(), interface ? interface->lastError().message() : QString() );
      } else {
        // FIXME: add correct item part list
        ok = interface->requestItemDelivery( uid, QString::fromUtf8(remote_id), QStringList() );
      }

      mPendingItemDeliveriesMutex.lock();
      qDebug() << "requestItemDelivery(): freeing uid" << uid;
      mPendingItemDeliveries.removeAll( uid );
      mPendingItemDeliveriesCondition.wakeAll();
      mPendingItemDeliveriesMutex.unlock();
  }

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
       << PimItem::datetimeColumn() << PimItem::remoteIdColumn() << PimItem::atimeColumn() << PimItem::dirtyColumn();
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
  QDateTime atime = dateTimeToQDateTime( query.value( 5 ).toByteArray() );
  bool dirty = query.value( 6 ).toBool();
  QByteArray data = query.value( 7 ).toByteArray();
  if ( data.isEmpty() && type != FetchQuery::FastType )
      data = retrieveDataFromResource( id, remote_id, location, type );

  // update access time
  PimItem item = PimItem( pimItemId, remote_id, data, location, mimetype, dateTime, atime, dirty );
  item.setAtime( QDateTime::currentDateTime() );
  if ( !item.update() )
    qDebug() << "Failed to update access time for item" << item.id();

  return item;
}

PimItem DataStore::pimItemById( int id )
{
  return pimItemById( id, FetchQuery::FastType );
}

QList<PimItem> DataStore::listPimItems( const Location & location, const Flag &flag )
{
  if ( !m_dbOpened )
    return QList<PimItem>();

  SelectQueryBuilder<PimItem> qb;
  qb.addTable( PimItemFlagRelation::tableName() );
  qb.addColumnCondition( PimItem::idFullColumnName(), "=", PimItemFlagRelation::leftFullColumnName() );
  qb.addValueCondition( PimItemFlagRelation::rightFullColumnName(), "=", flag.id() );

  if ( location.isValid() )
    qb.addValueCondition( PimItem::locationIdFullColumnName(), "=", location.id() );

   if ( !qb.exec() )
    return QList<PimItem>();

  return qb.result();
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

  if ( location.resource().name() == QLatin1String("akonadi_search_resource") ) {
    CountQueryBuilder builder;
    builder.addTable( LocationPimItemRelation::tableName() );
    builder.addValueCondition( LocationPimItemRelation::leftColumn(), "=", location.id() );
    if ( !builder.exec() )
      return -1;
    return builder.result();
  } else {
    int cnt = PimItem::count( PimItem::locationIdColumn(), location.id() );
    if ( cnt < 0 )
      return -1;

    return cnt + 1;
  }
}


QList<PimItem> Akonadi::DataStore::fetchMatchingPimItemsByUID( const FetchQuery &query, const Location& l )
{
    return matchingPimItemsByUID( query.sequences(), query.type(), l );
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

  QueryBuilder builder( QueryBuilder::Select );
  if ( location.resource().name() != QLatin1String( "akonadi_search_resource" ) ) {
    builder.addTable( PimItem::tableName() );
    builder.addValueCondition( PimItem::locationIdColumn(), "=", location.id() );
    builder.addColumn( PimItem::idColumn() );
  } else {
    builder.addTable( LocationPimItemRelation::tableName() );
    builder.addValueCondition( LocationPimItemRelation::leftColumn(), "=", location.id() );
    builder.addColumn( LocationPimItemRelation::rightColumn() );
  }

  if ( !builder.exec() )
    return PimItem::List();

  int highestEntry = highestPimItemCountByLocation( location );
  if ( highestEntry == -1 ) {
    qDebug( "DataStore::matchingPimItemsBySequenceNumbers: Invalid highest entry number '%d' ", highestEntry );
    return QList<PimItem>();
  }

  // iterate over the whole query to make seek possible.
  QSqlQuery query = builder.query();
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

  Resource res( resource, policy.id() );
  return res.insert();
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


bool DataStore::addCollectionAttribute(const Location & loc, const QByteArray & key, const QByteArray & value)
{
  SelectQueryBuilder<LocationAttribute> qb;
  qb.addValueCondition( LocationAttribute::locationIdColumn(), "=", loc.id() );
  qb.addValueCondition( LocationAttribute::typeColumn(), "=", key );
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
  qb.addValueCondition( LocationAttribute::locationIdColumn(), "=", loc.id() );
  qb.addValueCondition( LocationAttribute::typeColumn(), "=", key );
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
