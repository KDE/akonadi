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
#include "dbconfig.h"
#include "dbinitializer.h"
#include "dbupdater.h"
#include "notificationmanager.h"
#include "tracer.h"
#include "selectquerybuilder.h"
#include "handlerhelper.h"
#include "countquerybuilder.h"
#include "xdgbasedirs_p.h"
#include "akdebug.h"
#include "parthelper.h"
#include "libs/protocol_p.h"

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

/***************************************************************************
 *   DataStore                                                           *
 ***************************************************************************/
DataStore::DataStore() :
  QObject(),
  m_dbOpened( false ),
  m_transactionLevel( 0 ),
  mNotificationCollector( new NotificationCollector( this ) )
{
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

  m_database = QSqlDatabase::addDatabase( DbConfig::driverName(), m_connectionName );
  DbConfig::configure( m_database );

  if ( !m_database.isValid() ) {
    m_dbOpened = false;
    return;
  }
  m_dbOpened = m_database.open();

  if ( !m_dbOpened )
    debugLastDbError( "Cannot open database." );
  else
    qDebug() << "Database" << m_database.databaseName() << "opened using driver" << m_database.driverName();
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
    akError() << initializer.errorMsg();
    return false;
  }

  DbUpdater updater( m_database, QLatin1String( ":dbupdate.xml" ) );
  if ( !updater.run() )
    return false;

  // enable caching for some tables
  MimeType::enableCache( true );
  Flag::enableCache( true );
  Resource::enableCache( true );
  Collection::enableCache( true );

  return true;
}

QThreadStorage<DataStore*> instances;

DataStore * Akonadi::DataStore::self()
{
  if ( !instances.hasLocalData() )
    instances.setLocalData( new DataStore() );
  return instances.localData();
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

  mNotificationCollector->itemChanged( item, QSet<QByteArray>() << "FLAGS" );
  return true;
}

bool DataStore::appendItemFlags( const PimItem &item, const QList<Flag> &flags,
                                 bool checkIfExists, const Collection &col )
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

  mNotificationCollector->itemChanged( item, QSet<QByteArray>() << "FLAGS", col );
  return true;
}

bool DataStore::appendItemFlags( const PimItem &item, const QList<QByteArray> &flags,
                                 bool checkIfExists, const Collection &col )
{
  Flag::List list;
  foreach ( const QByteArray &f, flags ) {
    Flag flag = Flag::retrieveByName( QString::fromUtf8( f ) );
    if ( !flag.isValid() ) {
      flag = Flag( QString::fromUtf8( f ) );
      if ( !flag.insert() )
        return false;
    }
    list << flag;
  }
  return appendItemFlags( item, list, checkIfExists, col );
}

bool DataStore::removeItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  for ( int i = 0; i < flags.count(); ++i ) {
    if ( !item.removeFlag( flags[ i ] ) )
      return false;
  }

  mNotificationCollector->itemChanged( item, QSet<QByteArray>() << "FLAGS" );
  return true;
}

/* --- ItemParts ----------------------------------------------------- */

bool DataStore::removeItemParts( const PimItem &item, const QList<QByteArray> &parts )
{
  Part::List existingParts = item.parts();
  foreach ( Part part, existingParts ) {
    if( parts.contains( part.name().toLatin1() ) ) {
      if ( !PartHelper::remove(&part) )
        return false;
    }
  }

  mNotificationCollector->itemChanged( item, parts.toSet() );
  return true;
}

/* --- Collection ------------------------------------------------------ */
bool DataStore::appendCollection( Collection &collection )
{
  // no need to check for already existing collection with the same name,
  // a unique index on parent + name prevents that in the database
  if ( !collection.insert() )
    return false;

  mNotificationCollector->collectionAdded( collection );
  return true;
}

bool Akonadi::DataStore::cleanupCollection(Collection &collection)
{
  // delete the content
  PimItem::List items = collection.items();
  foreach ( PimItem item, items )
    cleanupPimItem( item );

  // delete collection mimetypes
  collection.clearMimeTypes();
  Collection::clearPimItems( collection.id() );

  // delete attributes
  foreach ( CollectionAttribute attr, collection.attributes() )
    if ( !attr.remove() )
      return false;

  // delete the collection itself
  mNotificationCollector->collectionRemoved( collection );
  return collection.remove();
}

static bool recursiveSetResourceId( const Collection & collection, qint64 resourceId )
{
  QueryBuilder qb( QueryBuilder::Update );
  qb.addTable( Collection::tableName() );
  qb.addValueCondition( Collection::parentIdColumn(), Query::Equals, collection.id() );
  qb.updateColumnValue( Collection::resourceIdColumn(), resourceId );
  if ( !qb.exec() )
    return false;
  foreach ( const Collection &col, collection.children() ) {
    if ( !recursiveSetResourceId( col, resourceId ) )
      return false;
  }
  return true;
}

bool Akonadi::DataStore::renameCollection( Collection & collection, qint64 newParent, const QByteArray & newName)
{
  if ( collection.name() == newName && collection.parentId() == newParent )
    return true;

  if ( !m_dbOpened )
    return false;

  int resourceId = collection.resourceId();
  if ( newParent > 0 && collection.parentId() != newParent ) {
    Collection parent = Collection::retrieveById( newParent );
    resourceId = parent.resourceId();
    if ( !parent.isValid() )
      return false;

    forever {
      if ( parent.id() == collection.id() )
        return false; // target is child of source
      if ( parent.parentId() == 0 )
        break;
      parent = parent.parent();
    }
  }

  SelectQueryBuilder<Collection> qb;
  qb.addValueCondition( Collection::parentIdColumn(), Query::Equals, newParent );
  qb.addValueCondition( Collection::nameColumn(), Query::Equals, newName );
  if ( !qb.exec() || qb.result().count() > 0 )
    return false;

  collection.setName( newName );
  collection.setParentId( newParent );
  if ( collection.resourceId() != resourceId ) {
    collection.setResourceId( resourceId );
    if ( !recursiveSetResourceId( collection, resourceId ) )
      return false;
  }

  if ( !collection.update() )
    return false;

  QList<QByteArray> changes = QList<QByteArray>() << "PARENT" << AKONADI_PARAM_NAME;
  mNotificationCollector->collectionChanged( collection, changes );
  return true;
}


bool DataStore::appendMimeTypeForCollection( qint64 collectionId, const QStringList & mimeTypes )
{
  if ( mimeTypes.isEmpty() )
    return true;
  SelectQueryBuilder<MimeType> qb;
  qb.addValueCondition( MimeType::nameColumn(), Query::In, mimeTypes );
  if ( !qb.exec() )
    return false;
  QStringList missingMimeTypes = mimeTypes;

  foreach ( const MimeType &mt, qb.result() ) {
    // unique index on n:m relation prevents duplicates, ie. this will fail
    // if this mimetype is already set
    if ( !Collection::addMimeType( collectionId, mt.id() ) )
      return false;
    missingMimeTypes.removeAll( mt.name() );
  }

  // the MIME type doesn't exist, so we have to add it to the db
  foreach ( const QString &mtName, missingMimeTypes ) {
    qint64 mimeTypeId;
    if ( !appendMimeType( mtName, &mimeTypeId ) )
      return false;
    if ( !Collection::addMimeType( collectionId, mimeTypeId ) )
      return false;
  }

  return true;
}


void DataStore::activeCachePolicy(Collection & col)
{
  if ( !col.cachePolicyInherit() )
    return;

  Collection parent = col.parent();
  while ( parent.isValid() ) {
    if ( !parent.cachePolicyInherit() ) {
      col.setCachePolicyCheckInterval( parent.cachePolicyCheckInterval() );
      col.setCachePolicyCacheTimeout( parent.cachePolicyCacheTimeout() );
      col.setCachePolicySyncOnDemand( parent.cachePolicySyncOnDemand() );
      col.setCachePolicyLocalParts( parent.cachePolicyLocalParts() );
      return;
    }
    parent = parent.parent();
  }

  // ### system default
  col.setCachePolicyCheckInterval( -1 );
  col.setCachePolicyCacheTimeout( -1 );
  col.setCachePolicySyncOnDemand( false );
  col.setCachePolicyLocalParts( QLatin1String("ALL") );
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
bool DataStore::appendPimItem( QList<Part> & parts,
                               const MimeType & mimetype,
                               const Collection & collection,
                               const QDateTime & dateTime,
                               const QString & remote_id,
                               PimItem &pimItem )
{
  pimItem.setMimeTypeId( mimetype.id() );
  pimItem.setCollectionId( collection.id() );
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
    //don't use foreach, the caller depends on knowing the part has changed, see the Append handler
    for(QList<Part>::iterator it = parts.begin(); it != parts.end(); ++it ) {

      (*it).setPimItemId( pimItem.id() );
      (*it).setDatasize( (*it).data().size() );

      qDebug() << "Insert from DataStore::appendPimItem";
      if( !PartHelper::insert(&(*it)) )
        return false;
    }
  }

  mNotificationCollector->itemAdded( pimItem, collection, mimetype.name() );
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
  if ( !PartHelper::remove( Part::pimItemIdColumn(), item.id() ) )
    return false;
  if ( !PimItem::remove( PimItem::idColumn(), item.id() ) )
    return false;

  if ( !Entity::clearRelation<CollectionPimItemRelation>( item.id(), Entity::Right ) )
    return false;

  return true;
}

bool DataStore::cleanupPimItems( const Collection &collection )
{
  if ( !m_dbOpened || !collection.isValid() )
    return false;

  QueryBuilder qb( QueryBuilder::Select );
  qb.addTable( Flag::tableName() );
  qb.addTable( PimItemFlagRelation::tableName() );
  qb.addTable( PimItem::tableName() );
  qb.addColumn( PimItemFlagRelation::leftFullColumnName() );
  qb.addValueCondition( Flag::nameFullColumnName(), Query::Equals, QLatin1String("\\Deleted") );
  qb.addValueCondition( PimItem::collectionIdFullColumnName(), Query::Equals, collection.id() );
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


bool DataStore::addCollectionAttribute(const Collection & col, const QByteArray & key, const QByteArray & value)
{
  SelectQueryBuilder<CollectionAttribute> qb;
  qb.addValueCondition( CollectionAttribute::collectionIdColumn(), Query::Equals, col.id() );
  qb.addValueCondition( CollectionAttribute::typeColumn(), Query::Equals, key );
  if ( !qb.exec() )
    return false;

  if ( qb.result().count() > 0 ) {
    qDebug() << "Attribute" << key << "already exists for collection" << col.id();
    return false;
  }

  CollectionAttribute attr;
  attr.setCollectionId( col.id() );
  attr.setType( key );
  attr.setValue( value );

  if ( !attr.insert() )
    return false;

  mNotificationCollector->collectionChanged( col, QList<QByteArray>() << key );
  return true;
}

bool Akonadi::DataStore::removeCollectionAttribute(const Collection & col, const QByteArray & key)
{
  SelectQueryBuilder<CollectionAttribute> qb;
  qb.addValueCondition( CollectionAttribute::collectionIdColumn(), Query::Equals, col.id() );
  qb.addValueCondition( CollectionAttribute::typeColumn(), Query::Equals, key );
  if ( !qb.exec() )
    return false;

  foreach ( CollectionAttribute attr, qb.result() ) {
    if ( !attr.remove() )
      return false;
  }

  mNotificationCollector->collectionChanged( col, QList<QByteArray>() << key );
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

#include "datastore.moc"
