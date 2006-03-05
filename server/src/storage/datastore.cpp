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

#include <QSqlQuery>
#include <QVariant>

namespace Akonadi {

/***************************************************************************
 *   DataStore                                                           *
 ***************************************************************************/
DataStore * DataStore::ds_instance = 0;

DataStore::DataStore()
{
  m_database = QSqlDatabase::addDatabase( "QSQLITE" );
  m_database.setDatabaseName( "akonadi.db" );
  m_dbOpened = m_database.open();
  if ( !m_dbOpened )
    debugLastDbError( "Cannot open database." );
  else
    qDebug() << "Database akonadi.db opened.";
}

DataStore::~DataStore()
{
}

/* --- CachePolicy --------------------------------------------------- */
bool DataStore::appendCachePolicy( const QString & policy )
{
  int foundRecs = 0;
  QSqlQuery query;
  if ( m_dbOpened ) {
    query.prepare( "SELECT COUNT(*) FROM CachePolicies WHERE name = :name" );
    query.bindValue(":name", policy);
    if ( query.exec() ) {
      if (query.next())
        foundRecs = query.value(0).toInt();
    } else
      debugLastDbError( "Error during check before insertion of CachePolicy." );
    if ( foundRecs == 0) {
      query.prepare( "INSERT INTO CachePolicies (name) VALUES (:name)" );
      query.bindValue(":name", policy);
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
    QSqlQuery query;
    query.prepare( "SELECT id, name FROM CachePolicies WHERE id = :id" );
    query.bindValue(":id", id);
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

QList<CachePolicy> * DataStore::listCachePolicies()
{
  QList<CachePolicy> * list = new QList<CachePolicy>();
  if ( m_dbOpened ) {
    QSqlQuery query;
    if ( query.exec( "SELECT id, name FROM CachePolicies" ) ) {
      while (query.next()) {
        int id = query.value(0).toInt();
        QString policy = query.value(1).toString();
        list->append( CachePolicy( id, policy ) );
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
/*
bool appendLocation( const QString & location, const Resource & resource );
bool appendLocation( const QString & location,
             const Resource & resource,
             const CachePolicy & policy );
bool removeLocation( const Location & location );
bool removeLocation( int id );
bool changeLocationPolicy( const Location & location, const CachePolicy & policy );
bool resetLocationPolicy( const Location & location );
Location * getLocationById( int id );
QList<MetaType> * listLocations();
QList<MetaType> * listLocations( const Resource & resource );
*/

/* --- MimeType ------------------------------------------------------ */
bool DataStore::appendMimeType( const QString & mimetype )
{
  int foundRecs = 0;
  QSqlQuery query;
  if ( m_dbOpened ) {
    query.prepare( "SELECT COUNT(*) FROM MimeTypes WHERE mime_type = :type" );
    query.bindValue(":type", mimetype);
    if ( query.exec() ) {
      if (query.next())
        foundRecs = query.value(0).toInt();
    } else
      debugLastDbError( "Error during check before insertion of MimeType." );
    if ( foundRecs == 0) {
      query.prepare( "INSERT INTO MimeTypes (mime_type) VALUES (:type)" );
      query.bindValue(":type", mimetype);
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
    QSqlQuery query;
    query.prepare( "SELECT id, mime_type FROM MimeTypes WHERE id = :id" );
    query.bindValue(":id", id);
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

QList<MimeType> * DataStore::listMimeTypes()
{
  QList<MimeType> * list = new QList<MimeType>();
  if ( m_dbOpened ) {
    QSqlQuery query;
    if ( query.exec( "SELECT id, mime_type FROM MimeTypes" ) ) {
      while (query.next()) {
        int id = query.value(0).toInt();
        QString type = query.value(1).toString();
        list->append( MimeType( id, type ) );
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

QList<MetaType> * DataStore::listMetaTypes()
{
  // TODO implement
  QList<MetaType> * list = new QList<MetaType>();
  list->append( *(getMetaTypeById( 1 )) );
  return list;
}

QList<MetaType> * DataStore::listMetaTypes( const MimeType & mimetype )
{
  // TODO implement
  QList<MetaType> * list = new QList<MetaType>();
  // let's fake it for now
  if ( mimetype.getId() == 1 )
    list->append( *(this->getMetaTypeById( 1 )) );
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

QList<PimItem> * DataStore::listPimItems( const MimeType & mimetype,
                                          const Location & location )
{
  // TODO implement
  QList<PimItem> * list = new QList<PimItem>();
  list->append( *(getPimItemById( 1 )) );
  return list;
}


/* --- Resource ------------------------------------------------------ */
bool DataStore::appendResource( const QString & resource,
                                const CachePolicy & policy )
{
  // TODO implement
  return false;
}

bool DataStore::removeResource( const Resource & resource )
{
  // TODO implement
  return false;
}

bool DataStore::removeResource( int id )
{
  // TODO implement
  return false;
}

Resource * DataStore::getResourceById( int id )
{
  // TODO implement
  Resource * r = new Resource();
  r->setId( id ).setResource( "dummyRecource" ).setPolicyId( 1 );
  return r;
}

QList<Resource> * DataStore::listResources()
{
  // TODO implement
  QList<Resource> * list = new QList<Resource>();
  list->append( *(getResourceById( 1 )) );
  return list;
}

QList<Resource> * DataStore::listResources( const CachePolicy & policy )
{
  // TODO implement
  QList<Resource> * list = new QList<Resource>();
  // let's fake it for now
  if ( policy.getId() == 1 )
    list->append( *(getResourceById( 1 )) );
  return list;
}


/* ------------------------------------------------------------------- */
DataStore * DataStore::instance()
{
  if ( !ds_instance )
    ds_instance = new DataStore();
  return ds_instance;
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
    QSqlQuery query;
    QString statement( "DELETE FROM ");
    statement += tableName + " WHERE id = :id";
    query.prepare( statement );
    query.bindValue(":id", id);
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
