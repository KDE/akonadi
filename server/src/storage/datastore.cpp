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
#include "persistentsearchmanager.h"

#include <QSqlQuery>
#include <QVariant>
#include <QThread>
#include <QUuid>
#include <QStringList>


namespace Akonadi {

/***************************************************************************
 *   DataStore                                                           *
 ***************************************************************************/
DataStore::DataStore()
{
  m_database = QSqlDatabase::addDatabase( "QSQLITE", QUuid::createUuid().toString() );
  m_database.setDatabaseName( "akonadi.db" );
  m_dbOpened = m_database.open();
  if ( !m_dbOpened )
    debugLastDbError( "Cannot open database." );
  else
      qDebug() << "Database akonadi.db opened.";
}

DataStore::~DataStore()
{
  m_database.close();
}

/* -- High level API -- */
const CollectionList Akonadi::DataStore::listCollections( const QByteArray & prefix,
                                                          const QByteArray & mailboxPattern ) const
{
    CollectionList result;

    if ( mailboxPattern.isEmpty() )
        return result;

    QByteArray sanitizedPattern( mailboxPattern );
    QByteArray fullPrefix( prefix );
    const bool hasPercent = mailboxPattern.contains('%');
    const bool hasStar = mailboxPattern.contains('*');
    if ( hasPercent || hasStar ) {
        int endOfPath = mailboxPattern.lastIndexOf('/') + 1;
            
        if ( mailboxPattern[0] != '/' && fullPrefix != "/" )
            fullPrefix += "/";
        fullPrefix += mailboxPattern.left( endOfPath );

        if ( hasPercent )
            sanitizedPattern = "%";
        if ( hasStar )
            sanitizedPattern = "*";
    }
    //qDebug() << "FullPrefix: " << fullPrefix << " pattern: " << sanitizedPattern;


    if ( fullPrefix == "/" )
    {
        // list resources and queries
        const QList<Resource> resources = listResources();
        foreach ( Resource r, resources )
        {
            Collection c( "/" + r.getResource() );
            result.append( c );
        }

        CollectionList persistenSearches = PersistentSearchManager::self()->collections();
        if ( !persistenSearches.isEmpty() )
            result.append( Collection( "/Search" ) );

    }
    else if ( fullPrefix == "Search" )
    {
        result += PersistentSearchManager::self()->collections();
    }
    else
    {
        const QByteArray resource = fullPrefix.mid( 1, fullPrefix.indexOf('/', 1) - 1 );
        qDebug() << "Listing folders in resource: " << resource;
        Resource r = getResourceByName( resource );
        const QList<Location> locations = listLocations( r );

        foreach( Location l, locations )
        {
            const QString location = "/" + resource + l.getLocation();
            //qDebug() << "Location: " << location << " " << resource << " prefix: " << fullPrefix;
            const bool atFirstLevel = location.lastIndexOf('/') == fullPrefix.lastIndexOf('/');
            if ( location.startsWith( fullPrefix ) ) {
                if ( hasStar || ( hasPercent && atFirstLevel ) ) {
                    Collection c( location );
                    result.append( c );
                }
            }
        }
    }
    return result;
}

/* --- CachePolicy --------------------------------------------------- */
bool DataStore::appendCachePolicy( const QString & policy )
{
  int foundRecs = 0;
  QSqlQuery query( m_database );
  if ( m_dbOpened ) {
    query.prepare( "SELECT COUNT(*) FROM CachePolicies WHERE name = :name" );
    query.bindValue( ":name", policy );
    if ( query.exec() ) {
      if (query.next())
        foundRecs = query.value(0).toInt();
    } else
      debugLastDbError( "Error during check before insertion of CachePolicy." );
    if ( foundRecs == 0) {
      query.prepare( "INSERT INTO CachePolicies (name) VALUES (:name)" );
      query.bindValue( ":name", policy );
      if ( query.exec() )
        return true;
      else
        debugLastDbError( "Error during insertion of single CachePolicy." );
    } else
      qDebug() << "Cannot insert policy " << policy
               << " because it already exists.";
  }
  return false;
}

bool DataStore::removeCachePolicy( const CachePolicy & policy )
{
  return removeCachePolicy( policy.getId() );
}

bool DataStore::removeCachePolicy( int id )
{
  return removeById( id, "CachePolicies" );
}

CachePolicy * DataStore::getCachePolicyById( int id )
{
  CachePolicy * p = 0;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    query.prepare( "SELECT id, name FROM CachePolicies WHERE id = :id" );
    query.bindValue( ":id", id );
    if ( query.exec() ) {
      if (query.next()) {
        int id = query.value(0).toInt();
        QString policy = query.value(1).toString();
        p = new CachePolicy( id, policy );
      }
    } else 
      debugLastDbError( "Error during selection of single CachePolicy." );
  }
  return p;
}

QList<CachePolicy> DataStore::listCachePolicies()
{
  QList<CachePolicy> list;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    if ( query.exec( "SELECT id, name FROM CachePolicies" ) ) {
      while (query.next()) {
        int id = query.value(0).toInt();
        QString policy = query.value(1).toString();
        list.append( CachePolicy( id, policy ) );
      }
    } else 
      debugLastDbError( "Error during selection of CachePolicies." );
  }
  return list;
}

/* --- ItemMetaData--------------------------------------------------- */
/*
bool appendItemMetaData( const QString & metadata, const MetaType & metatype );
bool removeItemMetaData( const ItemMetaData & metadata );
bool removeItemMetaData( int id );
ItemMetaData * getItemMetaDataById( int id );
QList<ItemMetaData> * listItemMetaData();
QList<ItemMetaData> * listItemMetaData( const MetaType & metatype );
*/

/* --- Location ------------------------------------------------------ */
bool DataStore::appendLocation( const QString & location,
                                const Resource & resource )
{
  int foundRecs = 0;
  QSqlQuery query( m_database );
  if ( m_dbOpened ) {
    query.prepare( "SELECT COUNT(*) FROM Locations WHERE uri = :uri" );
    query.bindValue( ":uri", location );
    if ( query.exec() ) {
      if (query.next())
        foundRecs = query.value(0).toInt();
    } else
      debugLastDbError( "Error during check before insertion of Location." );
    if ( foundRecs == 0) {
      query.prepare( "INSERT INTO Locations (uri, cachepolicy_id, resource_id) "
                     "VALUES (:uri, NULL, :resource)" );
      query.bindValue( ":uri", location );
      query.bindValue( ":resource", resource.getId() );
      if ( query.exec() )
        return true;
      else
        debugLastDbError( "Error during insertion of single Location." );
    } else
      qDebug() << "Cannot insert location " << location
               << " because it already exists.";
  }
  return false;
}

bool DataStore::appendLocation( const QString & location,
                                const Resource & resource,
                                const CachePolicy & policy )
{
  int foundRecs = 0;
  QSqlQuery query( m_database );
  if ( m_dbOpened ) {
    query.prepare( "SELECT COUNT(*) FROM Locations WHERE uri = :uri" );
    query.bindValue( ":uri", location );
    if ( query.exec() ) {
      if (query.next())
        foundRecs = query.value(0).toInt();
    } else
      debugLastDbError( "Error during check before insertion of Location." );
    if ( foundRecs == 0) {
      query.prepare( "INSERT INTO Locations (uri, cachepolicy_id, resource_id) "
                     "VALUES (:uri, :policy, :resource)" );
      query.bindValue( ":uri", location );
      query.bindValue( ":policy", policy.getId() );
      query.bindValue( ":resource", resource.getId() );
      if ( query.exec() )
        return true;
      else
        debugLastDbError( "Error during insertion of single Location." );
    } else
      qDebug() << "Cannot insert location " << location
               << " because it already exists.";
  }
  return false;
}

bool DataStore::removeLocation( const Location & location )
{
  return removeLocation( location.getId() );
}

bool DataStore::removeLocation( int id )
{
  return removeById( id, "Locations" );
}

bool DataStore::changeLocationPolicy( const Location & location,
                                      const CachePolicy & policy )
{
  QSqlQuery query( m_database );
  if ( m_dbOpened ) {
    query.prepare( "UPDATE Locations SET cachepolicy_id = :policy WHERE id = :id" );
    query.bindValue( ":policy", policy.getId() );
    query.bindValue( ":id", location.getId() );
    if ( query.exec() )
      return true;
    else
      debugLastDbError( "Error during setting the cache policy of a single Location." );
  }
  return false;
}

bool DataStore::resetLocationPolicy( const Location & location )
{
  QSqlQuery query( m_database );
  if ( m_dbOpened ) {
    query.prepare( "UPDATE Locations SET cachepolicy_id = NULL WHERE id = :id" );
    query.bindValue( ":id", location.getId() );
    if ( query.exec() )
      return true;
    else
      debugLastDbError( "Error during reset of the cache policy of a single Location." );
  }
  return false;
}

Location * DataStore::getLocationById( int id )
{
  Location * l = 0;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    query.prepare( "SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE id = :id" );
    query.bindValue( ":id", id );
    if ( query.exec() ) {
      if (query.next()) {
        int id = query.value(0).toInt();
        QString uri = query.value(1).toString();
        int policy = query.value(2).toInt();
        int resource = query.value(2).toInt();
        l = new Location( id, uri, policy, resource );
      }
    } else 
      debugLastDbError( "Error during selection of single Location." );
  }
  return l;
}

QList<Location> DataStore::listLocations() const
{
  QList<Location> list;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    if ( query.exec( "SELECT id, uri, cachepolicy_id, resource_id FROM Locations" ) ) {
      while (query.next()) {
        int id = query.value(0).toInt();
        QString uri = query.value(1).toString();
        int policy = query.value(2).toInt();
        int resource = query.value(2).toInt();
        list.append( Location( id, uri, policy, resource ) );
      }
    } else 
      debugLastDbError( "Error during selection of Locations." );
  }
  return list;
}

QList<Location> DataStore::listLocations( const Resource & resource ) const
{
  QList<Location> list;
  if ( !resource.isValid() ) return list;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    // query.prepare( "SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE resource_id = :id" );
    // query.bindValue( ":id", resource.getId() );
    const QString queryString =
            "SELECT id, uri, cachepolicy_id, resource_id FROM Locations WHERE resource_id = \"" + QString::number( resource.getId() ) + "\"";
    if ( query.exec( queryString ) ) {
      while (query.next()) {

        int id = query.value(0).toInt();
        QString uri = query.value(1).toString();
        int policy = query.value(2).toInt();
        int resource = query.value(2).toInt();
        list.append( Location( id, uri, policy, resource ) );
      }
    } else 
      debugLastDbError( "Error during selection of Locations from a Resource." );
  }
  return list;
}

/* --- MimeType ------------------------------------------------------ */
bool DataStore::appendMimeType( const QString & mimetype )
{
  int foundRecs = 0;
  QSqlQuery query( m_database );
  if ( m_dbOpened ) {
    query.prepare( "SELECT COUNT(*) FROM MimeTypes WHERE mime_type = :type" );
    query.bindValue( ":type", mimetype );
    if ( query.exec() ) {
      if (query.next())
        foundRecs = query.value(0).toInt();
    } else
      debugLastDbError( "Error during check before insertion of MimeType." );
    if ( foundRecs == 0) {
      query.prepare( "INSERT INTO MimeTypes (mime_type) VALUES (:type)" );
      query.bindValue( ":type", mimetype );
      if ( query.exec() )
        return true;
      else
        debugLastDbError( "Error during insertion of single MimeType." );
    } else
      qDebug() << "Cannot insert mimetype " << mimetype
               << " because it already exists.";
  }
  return false;
}

bool DataStore::removeMimeType( const MimeType & mimetype )
{
  return removeMimeType( mimetype.getId() );
}

bool DataStore::removeMimeType( int id )
{
  return removeById( id, "MimeTypes" );
}

MimeType * DataStore::getMimeTypeById( int id )
{
  MimeType * m = 0;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    query.prepare( "SELECT id, mime_type FROM MimeTypes WHERE id = :id" );
    query.bindValue( ":id", id );
    if ( query.exec() ) {
      if (query.next()) {
        int id = query.value(0).toInt();
        QString type = query.value(1).toString();
        m = new MimeType( id, type );
      }
    } else 
      debugLastDbError( "Error during selection of single MimeType." );
  }
  return m;
}

QList<MimeType> DataStore::listMimeTypes()
{
  QList<MimeType> list;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    if ( query.exec( "SELECT id, mime_type FROM MimeTypes" ) ) {
      while (query.next()) {
        int id = query.value(0).toInt();
        QString type = query.value(1).toString();
        list.append( MimeType( id, type ) );
      }
    } else 
      debugLastDbError( "Error during selection of MimeTypes." );
  }
  return list;
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

MetaType * DataStore::getMetaTypeById( int id )
{
  // TODO implement
  MetaType * m = new MetaType();
  m->setId( id ).setMetaType( "dummyMetaType" ).setMimeTypeId( 1 );
  return m;
}

QList<MetaType> DataStore::listMetaTypes()
{
  // TODO implement
  QList<MetaType> list;
  list.append( *(getMetaTypeById( 1 )) );
  return list;
}

QList<MetaType> DataStore::listMetaTypes( const MimeType & mimetype )
{
  // TODO implement
  QList<MetaType> list;
  // let's fake it for now
  if ( mimetype.getId() == 1 )
    list.append( *(this->getMetaTypeById( 1 )) );
  return list;
}


/* --- PimItem ------------------------------------------------------- */
bool DataStore::appendPimItem( const QByteArray & data,
                               const MimeType & mimetype,
                               const QString & location )
{
  // TODO implement
  return false;
}

bool DataStore::removePimItem( const PimItem & location )
{
  // TODO implement
  return false;
}

bool DataStore::removePimItem( int id )
{
  // TODO implement
  return false;
}

PimItem * DataStore::getPimItemById( int id )
{
  // TODO implement
  PimItem * p = new PimItem();
  p->setId( id ).setData( "dummyData" );
  p->setMimeTypeId( 1 ).setLocationId( 1 );
  return p;
}

QList<PimItem> DataStore::listPimItems( const MimeType & mimetype,
                                          const Location & location )
{
  // TODO implement
  QList<PimItem> list;
  list.append( *(getPimItemById( 1 )) );
  return list;
}


/* --- Resource ------------------------------------------------------ */
bool DataStore::appendResource( const QString & resource,
                                const CachePolicy & policy )
{
  int foundRecs = 0;
  QSqlQuery query( m_database );
  if ( m_dbOpened ) {
    query.prepare( "SELECT COUNT(*) FROM Resources WHERE name = :name" );
    query.bindValue( ":name", resource );
    if ( query.exec() ) {
      if (query.next())
        foundRecs = query.value(0).toInt();
    } else
      debugLastDbError( "Error during check before insertion of Resource." );
    if ( foundRecs == 0) {
      query.prepare( "INSERT INTO Resources (name, cachepolicy_id) "
                     "VALUES (:name, :policy)" );
      query.bindValue( ":name", resource );
      query.bindValue( ":policy", policy.getId() );
      if ( query.exec() )
        return true;
      else
        debugLastDbError( "Error during insertion of single Resource." );
    } else
      qDebug() << "Cannot insert resource " << resource
               << " because it already exists.";
  }
  return false;
}

bool DataStore::removeResource( const Resource & resource )
{
  return removeResource( resource.getId() );
}

bool DataStore::removeResource( int id )
{
  return removeById( id, "Resources" );
}

Resource DataStore::getResourceById( int id )
{
    Resource r;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    query.prepare( "SELECT id, name, cachepolicy_id FROM Resources WHERE id = :id" );
    query.bindValue( ":id", id );
    if ( query.exec() ) {
      if (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        int id_res = query.value(0).toInt();
        r = Resource( id, name, id_res );
      }
    } else 
      debugLastDbError( "Error during selection of single Resource." );
  }
  return r;
}

const Resource DataStore::getResourceByName( const QByteArray& name ) const
{
    Resource r;
    if ( m_dbOpened ) {
        QSqlQuery query( m_database );
       // query.prepare( "SELECT id, name, cachepolicy_id FROM Resources WHERE name = :name" );
       // query.bindValue( ":name", name );
        QString querystring = "SELECT id, name, cachepolicy_id FROM Resources WHERE name = \"" + name + "\"";
        if ( query.exec( querystring) ) {
            if (query.next()) {
                int id = query.value(0).toInt();
                QString name = query.value(1).toString();
                int id_res = query.value(0).toInt();
                r = Resource( id, name, id_res );
            }
        } else
            debugLastDbError( "Error during selection of single Resource." );
    }
    return r;
}

QList<Resource> DataStore::listResources() const
{
  QList<Resource> list;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    if ( query.exec( "SELECT id, name, cachepolicy_id FROM Resources" ) ) {
      while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        int id_res = query.value(0).toInt();
        list.append( Resource( id, name, id_res ) );
      }
    } else 
      debugLastDbError( "Error during selection of Resources." );
  }
  return list;
}

QList<Resource> DataStore::listResources( const CachePolicy & policy )
{
  QList<Resource> list;
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    query.prepare( "SELECT id, name, cachepolicy_id FROM Resources WHERE cachepolicy_id = :id" );
    query.bindValue( ":id", policy.getId() );
    if ( query.exec() ) {
      while (query.next()) {
        int id = query.value(0).toInt();
        QString name = query.value(1).toString();
        int id_res = query.value(0).toInt();
        list.append( Resource( id, name, id_res ) );
      }
    } else 
      debugLastDbError( "Error during selection of Resources by a Policy." );
  }
  return list;
}


void DataStore::debugLastDbError( const QString & actionDescription ) const
{
  qDebug() << actionDescription
           << "\nDriver said: "
           << m_database.lastError().driverText()
           << "\nDatabase said: "
           << m_database.lastError().databaseText();
}

bool DataStore::removeById( int id, const QString & tableName )
{
  if ( m_dbOpened ) {
    QSqlQuery query( m_database );
    QString statement( "DELETE FROM ");
    statement += tableName + " WHERE id = :id";
    query.prepare( statement );
    query.bindValue( ":id", id );
    if ( query.exec() )
      return true;
    else {
      QString msg( "Error during deletion of a single row by ID from table " );
      msg += tableName + ".";
      debugLastDbError( msg );
    }
  }
  return false;
}

}
