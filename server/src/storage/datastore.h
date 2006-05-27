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

#ifndef DATASTORE_H
#define DATASTORE_H

#include <QList>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtDebug>

#include "entity.h"
#include "collection.h"

namespace Akonadi {

/***************************************************************************
 *   DataStore                                                             *
 ***************************************************************************/
class DataStore
{
public:
    DataStore();
    ~DataStore();

    /* -- higher level API -- */
    const CollectionList listCollections( const QByteArray& prefix,
                                          const QByteArray & mailboxPattern ) const;

    /* --- CachePolicy --------------------------------------------------- */
    bool appendCachePolicy( const QString & policy );
    bool removeCachePolicy( const CachePolicy & policy );
    bool removeCachePolicy( int id );
    CachePolicy * getCachePolicyById( int id );
    QList<CachePolicy> listCachePolicies();

    /* --- Flag ---------------------------------------------------------- */
    bool appendFlag( const QString & name );
    bool removeFlag( const Flag & flag );
    bool removeFlag( int id );
    Flag * getFlagById( int id );
    QList<Flag> listFlags();

    /* --- ItemMetaData--------------------------------------------------- */
    bool appendItemMetaData( const QString & metadata, const MetaType & metatype );
    bool removeItemMetaData( const ItemMetaData & metadata );
    bool removeItemMetaData( int id );
    ItemMetaData * getItemMetaDataById( int id );
    QList<ItemMetaData> listItemMetaData();
    QList<ItemMetaData> listItemMetaData( const MetaType & metatype );

    /* --- Location ------------------------------------------------------ */
    bool appendLocation( const QString & location, const Resource & resource,
                         int *insertId = 0 );
    bool appendLocation( const QString & location,
                 const Resource & resource,
                 const CachePolicy & policy );
    bool removeLocation( const Location & location );
    bool removeLocation( int id );
    bool changeLocationPolicy( const Location & location, const CachePolicy & policy );
    bool resetLocationPolicy( const Location & location );
    Location getLocationById( int id ) const;
    Location getLocationByName( const Resource&, const QByteArray& name ) const;
    Location getLocationByRawMailbox( const QByteArray& mailbox ) const;
    QList<Location> listLocations() const;
    QList<Location> listLocations( const Resource & resource ) const;
    QList<MimeType> getMimeTypesForLocation( int id ) const;
    bool appendMimeTypeForLocation( int locationId, const MimeType & mimeType );
    
    /* --- MimeType ------------------------------------------------------ */
    bool appendMimeType( const QString & mimetype );
    bool removeMimeType( const MimeType & mimetype );
    bool removeMimeType( int id );
    MimeType getMimeTypeById( int id ) const;
    QList<MimeType> listMimeTypes();

    /* --- MetaType ------------------------------------------------------ */
    bool appendMetaType( const QString & metatype, const MimeType & mimetype );
    bool removeMetaType( const MetaType & metatype );
    bool removeMetaType( int id );
    MetaType * getMetaTypeById( int id );
    QList<MetaType> listMetaTypes();
    QList<MetaType> listMetaTypes( const MimeType & mimetype );

    /* --- PimItem ------------------------------------------------------- */
    bool appendPimItem( const QByteArray & data,
                        const MimeType & mimetype,
                        const Location & location );
    bool removePimItem( const PimItem & pimItem );
    bool removePimItem( int id );
    PimItem getPimItemById( int id );
    QList<PimItem> listPimItems( const MimeType & mimetype,
                                 const Location & location );

    /* --- Resource ------------------------------------------------------ */
    bool appendResource( const QString & resource, const CachePolicy & policy );
    bool removeResource( const Resource & resource );
    bool removeResource( int id );
    Resource getResourceById( int id );
    const Resource getResourceByName( const QByteArray& name ) const;
    QList<Resource> listResources() const;
    QList<Resource> listResources( const CachePolicy & policy );

protected:
    void debugLastDbError( const QString & actionDescription ) const;
    void debugLastQueryError( const QSqlQuery &query, const QString & actionDescription ) const;
    bool removeById( int id, const QString & tableName );

private:
    static DataStore * ds_instance;

    QSqlDatabase m_database;
    bool m_dbOpened;
};

}
#endif
