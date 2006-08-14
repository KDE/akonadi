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

#include <QDir>
#include <QSqlQuery>
#include <QEventLoop>
#include <QSqlField>
#include <QSqlError>
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QVariant>
#include <QThread>
#include <QUuid>
#include <QString>
#include <QStringList>

#include "agentmanagerinterface.h"
#include "dbinitializer.h"
#include "notificationmanager.h"
#include "persistentsearchmanager.h"
#include "tracer.h"

#include "datastore.h"

using namespace Akonadi;

/***************************************************************************
 *   DataStore                                                           *
 ***************************************************************************/
DataStore::DataStore()
  : QObject()
{
}

void DataStore::init()
{
  m_database = QSqlDatabase::addDatabase( "QSQLITE", QUuid::createUuid().toString() );
  m_database.setDatabaseName( storagePath() );
  m_dbOpened = m_database.open();

  if ( !m_dbOpened )
    debugLastDbError( "Cannot open database." );
  else
    qDebug() << "Database akonadi.db opened.";

  DbInitializer initializer( m_database, ":akonadidb.xml" );
  if ( !initializer.run() ) {
    Tracer::self()->error( "DataStore::init()", QString( "Unable to initialize database: %1" ).arg( initializer.errorMsg() ) );
  }

  NotificationManager::self()->connectDatastore( this );
}

DataStore::~DataStore()
{
  m_database.close();
}

QString DataStore::storagePath()
{
  const QString akonadiHomeDir = QDir::homePath() + QDir::separator() + ".akonadi" + QDir::separator();
  if ( !QDir( akonadiHomeDir ).exists() ) {
    QDir dir;
    dir.mkdir( akonadiHomeDir );
  }

  const QString akonadiPath = akonadiHomeDir + "akonadi.db";

  if ( !QFile::exists( akonadiPath ) ) {
    QFile file( akonadiPath );
    if ( !file.open( QIODevice::WriteOnly ) ) {
      Tracer::self()->error( "DataStore::storagePath", QString( "Unable to create file '%1'" ).arg( akonadiPath ) );
    } else {
      file.close();
    }
  }

  return akonadiPath;
}

/* -- High level API -- */
CollectionList Akonadi::DataStore::listCollections( const QByteArray & prefix,
                                                    const QByteArray & mailboxPattern ) const
{
  CollectionList result;

  if ( mailboxPattern.isEmpty() )
    return result;

  // normalize path and pattern
  QByteArray sanitizedPattern( mailboxPattern );
  QByteArray fullPrefix( prefix );
  const bool hasPercent = mailboxPattern.contains('%');
  const bool hasStar = mailboxPattern.contains('*');
  const int endOfPath = mailboxPattern.lastIndexOf('/') + 1;

  if ( mailboxPattern[0] != '/' && fullPrefix != "/" )
    fullPrefix += "/";
  fullPrefix += mailboxPattern.left( endOfPath );

  if ( hasPercent )
    sanitizedPattern = "%";
  else if ( hasStar )
    sanitizedPattern = "*";
  else
    sanitizedPattern = mailboxPattern.mid( endOfPath );

  qDebug() << "FullPrefix: " << fullPrefix << " pattern: " << sanitizedPattern;

  if ( !fullPrefix.isEmpty() ) {
    result.setValid( false );
  }

  const QList<Location> locations = listLocations();
  foreach( Location l, locations ) {
    const QString location = "/" + l.location();
#if 0
    qDebug() << "Location: " << location << " l: " << l << " prefix: " << fullPrefix;
#endif
    const bool atFirstLevel =
      location.lastIndexOf( '/' ) == fullPrefix.lastIndexOf( '/' );
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
    if ( location + "/" == fullPrefix || fullPrefix == "/" )
      result.setValid( true );
  }

  // list queries
  if ( fullPrefix == "/" ) {
    CollectionList persistenSearches = PersistentSearchManager::self()->collections();
    if ( !persistenSearches.isEmpty() )
      result.append( Collection( "Search" ) );
    result.setValid( true );
  }
  if ( fullPrefix == "/Search/"  || (fullPrefix == "/" && hasStar) ) {
    result += PersistentSearchManager::self()->collections();
    result.setValid( true );
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
  QString statement = QString( "SELECT COUNT(*) FROM Flags WHERE name = '%1'" ).arg( name );
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
  statement = QString( "INSERT INTO Flags (name) VALUES ('%1')" ).arg( name );

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
  return removeById( id, "Flags" );
}

Flag DataStore::flagById( int id )
{
  if ( !m_dbOpened )
    return Flag();

  QSqlQuery query( m_database );
//  query.prepare( "SELECT id, name FROM Flags WHERE id = :id" );
//  query.bindValue( ":id", id );
  const QString statement = QString( "SELECT id, name FROM Flags WHERE id = %1" ).arg( id );

  if ( !query.exec( statement ) ) {
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
//  query.prepare( "SELECT id, name FROM Flags WHERE name = :id" );
//  query.bindValue( ":id", id );
  const QString statement = QString( "SELECT id, name FROM Flags WHERE name = \"%1\"" ).arg( name );

  if ( !query.exec( statement ) ) {
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
  if ( !query.exec( "SELECT id, name FROM Flags" ) ) {
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
  const QString statement = QString( "DELETE FROM ItemFlags WHERE pimitem_id = %1" ).arg( item.id() );
  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during deletion of ItemFlags." );
    return false;
  }

  // then add the new flags
  for ( int i = 0; i < flags.count(); ++i ) {
    const QString statement = QString( "INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES ( '%1', '%2' )" )
                                     .arg( flags[ i ].id() ).arg( item.id() );
    if ( !query.exec( statement ) ) {
      debugLastQueryError( query, "Error during adding new ItemFlags." );
      return false;
    }
  }

  return true;
}

bool DataStore::appendItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  for ( int i = 0; i < flags.count(); ++i ) {
    const QString statement = QString( "SELECT COUNT(*) FROM ItemFlags WHERE flag_id = %1 AND pimitem_id = %2" )
                                     .arg( flags[ i ].id() ).arg( item.id() );
    if ( !query.exec( statement ) ) {
      debugLastQueryError( query, "Error during select on ItemFlags." );
      return false;
    }

    query.next();
    int exists = query.value( 0 ).toInt();

    if ( !exists ) {
      const QString statement = QString( "INSERT INTO ItemFlags (flag_id, pimitem_id) VALUES ( '%1', '%2' )" )
                                       .arg( flags[ i ].id() ).arg( item.id() );
      if ( !query.exec( statement ) ) {
        debugLastQueryError( query, "Error during adding new ItemFlags." );
        return false;
      }
    }
  }

  return true;
}

bool DataStore::appendItemFlags( int pimItemId, const QList<QByteArray> &flags )
{
    // FIXME Implement me
    return false;
}

bool DataStore::removeItemFlags( const PimItem &item, const QList<Flag> &flags )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  for ( int i = 0; i < flags.count(); ++i ) {
    const QString statement = QString( "DELETE FROM ItemFlags WHERE flag_id = %1 AND pimitem_id = %2" )
                                     .arg( flags[ i ].id() ).arg( item.id() );
    if ( !query.exec( statement ) ) {
      debugLastQueryError( query, "Error during deletion of ItemFlags." );
      return false;
    }
  }

  return true;
}

QList<Flag> DataStore::itemFlags( const PimItem &item )
{
  QList<Flag> flags;

  if ( !m_dbOpened )
    return flags;

  QSqlQuery query( m_database );
  const QString statement = QString( "SELECT Flags.id, Flags.name FROM Flags, ItemFlags WHERE Flags.id = ItemFlags.flag_id AND ItemFlags.pimitem_id = %1" )
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

  query.prepare( "SELECT COUNT(*) FROM CachePolicies WHERE name = :name" );
  query.bindValue( ":name", policy );

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

  query.prepare( "INSERT INTO CachePolicies (name) VALUES (:name)" );
  query.bindValue( ":name", policy );

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
  return removeById( id, "CachePolicies" );
}

CachePolicy DataStore::cachePolicyById( int id )
{
  if ( !m_dbOpened )
    return CachePolicy();

  QSqlQuery query( m_database );
  query.prepare( "SELECT id, name FROM CachePolicies WHERE id = :id" );
  query.bindValue( ":id", id );

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
  if ( !query.exec( "SELECT id, name FROM CachePolicies" ) ) {
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

  if ( !query.exec( "SELECT COUNT(*) FROM Locations WHERE uri = \"" + location + "\"" ) ) {
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

  QString statement = QString( "INSERT INTO Locations (uri, cachepolicy_id, "
                               "resource_id, exists_count, recent_count, "
                               "unseen_count, first_unseen, uid_validity) "
                               "VALUES (\"%1\", NULL, \"%2\", 0, 0, 0, 0, 0 )" )
                               .arg( location ).arg( resource.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during insertion of single Location." );
    return false;
  }

  int id = lastInsertId( query );
  if ( insertId )
      *insertId = id;

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

  query.prepare( "SELECT COUNT(*) FROM Locations WHERE uri = :uri" );
  query.bindValue( ":uri", location );
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

  query.prepare( "INSERT INTO Locations (uri, cachepolicy_id, resource_id) "
                 "VALUES (:uri, :policy, :resource)" );
  query.bindValue( ":uri", location );
  query.bindValue( ":policy", policy.id() );
  query.bindValue( ":resource", resource.id() );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during insertion of single Location." );
    return false;
  }

  return true;
}

bool DataStore::removeLocation( const Location & location )
{
  return removeLocation( location.id() );
}

bool DataStore::removeLocation( int id )
{
  return removeById( id, "Locations" );
}

static void addToUpdateAssignments( QStringList & l, int change, const QString & name )
{
    if ( change > 0 )
        // return a = a + n
        l.append( name + " = " + name + " + " + QString::number( change ) );
    else if ( change < 0 )
        // return a = a - |n|
        l.append( name + " = " + name + " - " + QString::number( -change ) );
}

bool DataStore::updateLocationCounts( const Location & location, int existsChange,
                                      int recentChange, int unseenChange )
{
    if ( existsChange == 0 && recentChange == 0 && unseenChange == 0 )
        return true; // there's nothing to do

    QSqlQuery query( m_database );
    if ( m_dbOpened ) {
        QStringList assignments;
        addToUpdateAssignments( assignments, existsChange, "exists_count" );
        addToUpdateAssignments( assignments, recentChange, "recent_count" );
        addToUpdateAssignments( assignments, unseenChange, "unseen_count" );
        QString q = QString( "UPDATE Locations SET %1 WHERE id = \"%2\"" )
                    .arg( assignments.join(",") )
                    .arg( location.id() );
        qDebug() << "Executing SQL query " << q;
        if ( query.exec( q ) )
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

  query.prepare( "UPDATE Locations SET cachepolicy_id = :policy WHERE id = :id" );
  query.bindValue( ":policy", policy.id() );
  query.bindValue( ":id", location.id() );
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

  query.prepare( "UPDATE Locations SET cachepolicy_id = NULL WHERE id = :id" );
  query.bindValue( ":id", location.id() );
  if ( !query.exec() ) {
    debugLastQueryError( query, "Error during reset of the cache policy of a single Location." );
    return false;
  }

  return true;
}

Location DataStore::locationById( int id ) const
{
  if ( !m_dbOpened )
    return Location();

  QSqlQuery query( m_database );
  query.prepare( "SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE id = :id" );
  query.bindValue( ":id", id );
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

Location DataStore::locationByRawMailbox( const QByteArray& mailbox ) const
{
  return locationByName( mailbox );
}

QList<MimeType> DataStore::mimeTypesForLocation( int id ) const
{
  if ( !m_dbOpened )
    return QList<MimeType>();

  QSqlQuery query( m_database );
  const QString statement = QString( "SELECT mimetype_id FROM LocationMimeTypes WHERE location_id = %1" ).arg( id );

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

  QString statement = QString( "SELECT COUNT(*) FROM LocationMimeTypes WHERE location_id = \"%1\" AND mimetype_id = \"%2\"" )
                             .arg( locationId ).arg( mimeTypeId );

  if ( !query.exec( statement ) ) {
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

  statement = QString( "INSERT INTO LocationMimeTypes (location_id, mimetype_id) VALUES (\"%1\", \"%2\")" )
                     .arg( locationId ).arg( mimeTypeId );

  if ( !query.exec( statement ) ) {
    debugLastDbError( "Error during insertion of single LocationMimeType." );
    return false;
  }

  return true;
}


QList<Location> DataStore::listLocations() const
{
  if ( !m_dbOpened )
    return QList<Location>();

  QSqlQuery query( m_database );
  const QString statement = QString( "SELECT id, uri, cachepolicy_id, resource_id, exists_count, recent_count, unseen_count, first_unseen, uid_validity FROM Locations" );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during selection of Locations." );
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

QList<Location> DataStore::listLocations( const Resource & resource ) const
{
  if ( !m_dbOpened || !resource.isValid() )
    return QList<Location>();

  QSqlQuery query( m_database );
  // query.prepare( "SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE resource_id = :id" );
  // query.bindValue( ":id", resource.id() );

  const QString statement = QString( "SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE resource_id = %1" )
                                   .arg( resource.id() );

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

  const QString statement = QString( "SELECT COUNT(*) FROM MimeTypes WHERE mime_type = \"%1\"" )
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

  query.prepare( "INSERT INTO MimeTypes (mime_type) VALUES (:type)" );
  query.bindValue( ":type", mimetype );

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
  return removeById( id, "MimeTypes" );
}

MimeType DataStore::mimeTypeById( int id ) const
{
  if ( !m_dbOpened )
    return MimeType();

  QSqlQuery query( m_database );
  const QString statement = QString( "SELECT id, mime_type FROM MimeTypes WHERE id = %1" ).arg( id );

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
  const QString statement = QString( "SELECT id, mime_type FROM MimeTypes WHERE mime_type = \"" + name + "\"" );

  if ( !query.exec( statement ) ) {
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
  if ( !query.exec( "SELECT id, mime_type FROM MimeTypes" ) ) {
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
  m.setMetaType( "dummyMetaType" );
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

  QSqlField field( "data", QVariant::String );
  field.setValue( data );
  QString escaped = m_database.driver()->formatValue( field );
  QString statement;
  if ( dateTime.isValid() ) {
      statement = QString( "INSERT INTO PimItems (remote_id, data, location_id, mimetype_id, "
                           "datetime ) "
                           "VALUES ('%1', %2, %3, %4, '%5')")
                         .arg( QString( remote_id ) )
                         .arg( escaped )
                         .arg( location.id() )
                         .arg( mimetype.id() )
                         .arg( dateTimeFromQDateTime( dateTime ) );
  }
  else {
      // the database will use the current date/time for datetime
      statement = QString( "INSERT INTO PimItems (remote_id, data, location_id, mimetype_id) "
                           "VALUES ('%1', %2, %3, %4)")
                         .arg( QString( remote_id ) )
                         .arg( escaped )
                         .arg( location.id() )
                         .arg( mimetype.id() );
  }

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during insertion of single PimItem." );
    return false;
  }

  int id = lastInsertId( query );
  if ( insertId )
      *insertId = id;

  emit itemAdded( id, location.location().toLatin1() );

  return true;
}

bool Akonadi::DataStore::updatePimItem(const PimItem & pimItem, const QByteArray & data)
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );

  QSqlField field( "data", QVariant::String );
  field.setValue( data );
  QString escaped = m_database.driver()->formatValue( field );
  QString statement;
  statement = QString( "UPDATE PimItems SET data = %1 WHERE id = %2" )
      .arg( escaped )
      .arg( pimItem.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "Error during insertion of single PimItem." );
    return false;
  }

  // TODO
  //emit itemChanged( pimItem.id(), location?? );

  return true;
}

bool DataStore::removePimItem( const PimItem & pimItem )
{
  return removePimItem( pimItem.id() );
}

bool DataStore::removePimItem( int id )
{
  return removeById( id, "PimItems" );
}

bool DataStore::cleanupPimItem( const PimItem &item )
{
  if ( !m_dbOpened || !item.isValid() )
    return false;

  QSqlDriver *driver = m_database.driver();
  if ( !driver->beginTransaction() ) {
    debugLastDbError( "DataStore::cleanupPimItem: beginTransaction" );
    return false;
  }

  QSqlQuery query( m_database );

  QString statement = QString( "DELETE FROM ItemMetaData WHERE pimitem_id = %1" ).arg( item.id() );
  query.exec( statement );
  statement = QString( "DELETE FROM ItemFlags WHERE pimitem_id = %1" ).arg( item.id() );
  query.exec( statement );
  statement = QString( "DELETE FROM Parts WHERE pimitem_id = %1" ).arg( item.id() );
  query.exec( statement );
  statement = QString( "DELETE FROM PimItems WHERE id = %1" ).arg( item.id() );
  query.exec( statement );

  if ( !driver->commitTransaction() ) {
    debugLastDbError( "DataStore::cleanupPimItem: commitTransaction" );

    if ( !driver->rollbackTransaction() )
      debugLastDbError( "DataStore::cleanupPimItem: rollbackTransaction" );

    return false;
  }

  return true;
}

bool DataStore::cleanupPimItems( const Location &location )
{
  if ( !m_dbOpened || !location.isValid() )
    return false;

  const QString statement = QString( "SELECT ItemFlags.pimitem_id FROM Flags, ItemFlags, PimItems "
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

  const QString statement = QString( "SELECT id FROM PimItems WHERE location_id = %1" ).arg( item.locationId() );
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
  return "data";
}

QByteArray Akonadi::DataStore::retrieveDataFromResource( const QByteArray& remote_id,
                                                         int locationid, FetchQuery::Type type )
{
  org::kde::Akonadi::AgentManager agentManager( "org.kde.Akonadi.AgentManager", "/", QDBus::sessionBus() );
  // FIXME figure out the resource, ask it for the item, by remote id
  Location l = locationById( locationid );
  Resource r = resourceById( l.resourceId() );
  QByteArray data;
  if ( agentManager.requestItemDelivery( r.resource(), l.location(), remote_id, type ) ) {
    // wait for the delivery to be done...
    //QEventLoop loop( this );
   // connect( this, SIGNAL( done( PIM::Job* ) ), &loop, SLOT( quit( ) ) );
   // loop.exec( );
  }

  return data;
}


PimItem Akonadi::DataStore::pimItemById( int id, FetchQuery::Type type )
{
  if ( !m_dbOpened )
    return PimItem();

  const QString field = fieldNameForDataType( type );
  QSqlQuery query( m_database );
  query.prepare( QString( "SELECT id, %1, location_id, mimetype_id, datetime, remote_id FROM PimItems WHERE id = :id" ).arg( field ) );
  query.bindValue( ":id", id );

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
  QVariant v = query.value( 1 );
  QByteArray data;
  if ( v.isValid() ) {
      data = v.toByteArray();
  } else {
      data = retrieveDataFromResource( remote_id, location, type );
  }

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

  QString statement = QString( "SELECT PimItems.id, PimItems.data, PimItems.mimetype_id, PimItems.datetime, PimItems.remote_id FROM PimItems, ItemFlags WHERE "
                                     "ItemFlags.pimitem_id = PimItems.id AND ItemFlags.flag_id = %1" )
                                   .arg( flag.id() );
  if ( location.isValid() )
    statement += QString( "AND location_id = %1" ).arg( location.id() );

  if ( !query.exec( statement ) ) {
    debugLastQueryError( query, "DataStore::listPimItems" );
    return QList<PimItem>();
  }

  QList<PimItem> pimItems;
  while ( query.next() ) {
    pimItems.append( PimItem( query.value( 0 ).toInt(),
                              query.value( 4 ).toByteArray(),
                              query.value( 1 ).toByteArray(),
                              location.id(), query.value( 2 ).toInt(),
                              dateTimeToQDateTime( query.value( 3 ).toByteArray() ) ) );
  }

  return pimItems;
}

int DataStore::highestPimItemId() const
{
  if ( !m_dbOpened )
    return -1;

  QSqlQuery query( m_database );
  const QString statement = QString( "SELECT MAX(id) FROM PimItems" );

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
  const QString statement = QString( "SELECT COUNT(*) AS count FROM PimItems WHERE location_id = %1" ).arg( location.id() );

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
      const QString left( pair[ 0 ] );
      const QString right( pair[ 1 ] );

      if ( left == "*" && right == "*" ) {
        statementParts.append( QString( "id = %1" ).arg( QString::number( highestEntry ) ) );
      } else if ( left == "*" ) {
        statementParts.append( QString( "(id >=1 AND id <= %1)" ).arg( right ) );
      } else if ( right == "*" ) {
        statementParts.append( QString( "(id >=%1 AND id <= %2)" ).arg( left ).arg( highestEntry ) );
      } else {
        statementParts.append( QString( "(id >=%1 AND id <= %2)" ).arg( left, right ) );
      }
    } else {
      statementParts.append( QString( "id = %1" ).arg( QString::fromLatin1( sequences[ i ] ) ) );
    }
  }

  QString statement = QString( "SELECT id FROM PimItems WHERE (%1)" ).arg( statementParts.join( " OR " ) );
  if ( location.isValid() ) {
     statement += QString( " AND location_id = %1" ).arg( location.id() );
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
      QString( "SELECT id FROM PimItems WHERE location_id = %1" ).arg( location.id() );

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
  query.prepare( "SELECT COUNT(*) FROM Resources WHERE name = :name" );
  query.bindValue( ":name", resource );

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

  query.prepare( "INSERT INTO Resources (name, cachepolicy_id) "
                 "VALUES (:name, :policy)" );
  query.bindValue( ":name", resource );
  query.bindValue( ":policy", policy.id() );

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
  return removeById( id, "Resources" );
}

Resource DataStore::resourceById( int id )
{
  if ( !m_dbOpened )
    return Resource();

  QSqlQuery query( m_database );
  QString statement = QString( "SELECT id, name, cachepolicy_id FROM Resources WHERE id = %1" ).arg( id );

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
  // query.prepare( "SELECT id, name, cachepolicy_id FROM Resources WHERE name = :name" );
  // query.bindValue( ":name", name );
  const QString statement = QString( "SELECT id, name, cachepolicy_id FROM Resources WHERE name = \"" + name + "\"" );

  if ( !query.exec( statement ) ) {
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
  if ( !query.exec( "SELECT id, name, cachepolicy_id FROM Resources" ) ) {
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
  query.prepare( "SELECT id, name, cachepolicy_id FROM Resources WHERE cachepolicy_id = :id" );
  query.bindValue( ":id", policy.id() );

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


void DataStore::debugLastDbError( const QString & actionDescription ) const
{
  qDebug() << actionDescription
           << "\nDriver said: "
           << m_database.lastError().driverText()
           << "\nDatabase said: "
           << m_database.lastError().databaseText();
}

void DataStore::debugLastQueryError( const QSqlQuery &query, const QString & actionDescription ) const
{
  qDebug() << actionDescription
           << ": " << query.lastError().text();
}

bool DataStore::removeById( int id, const QString & tableName )
{
  if ( !m_dbOpened )
    return false;

  QSqlQuery query( m_database );
  const QString statement = QString( "DELETE FROM %1 WHERE id = :id" ).arg( tableName );
  query.prepare( statement );
  query.bindValue( ":id", id );

  if ( !query.exec() ) {
    QString msg = QString( "Error during deletion of a single row by ID from table %1: " ).arg( tableName );
    debugLastQueryError( query, msg );
    return false;
  }

  return true;
}

Location DataStore::locationByName( const QByteArray & name ) const
{
  qDebug() << "locationByName( " << name << " )";

  if ( !m_dbOpened )
    return Location();

  QSqlQuery query( m_database );
  const QString statement = QString( "SELECT id, uri, cachepolicy_id, resource_id, exists_count, recent_count,"
                                     " unseen_count, first_unseen, uid_validity FROM Locations "
                                     "WHERE uri = \"%2\"" ).arg( QString::fromLatin1( name ) );

  if ( !query.exec( statement ) ) {
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
    return utcDateTime.toString( "yyyy-MM-dd hh:mm:ss" );
}


// static
QDateTime DataStore::dateTimeToQDateTime( const QByteArray & dateTime )
{
    return QDateTime::fromString( dateTime, "yyyy-MM-dd hh:mm:ss" );
}

#include "datastore.moc"
