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

#ifndef ENTITY_H
#define ENTITY_H

#include <Qt>
#include <QDebug>
#include <QString>

namespace Akonadi {

class DataStore;

class CachePolicy;
class ItemMetaData;
class Location;
class MimeType;
class MetaType;
class PimItem;
class Resource;

/***************************************************************************
 *   CachePolicy                                                           *
 ***************************************************************************/
class CachePolicy
{
public:
    CachePolicy();
    CachePolicy( int id, const QString & policy );
    ~CachePolicy();

    int getId() const { return m_id; };
    const QString & getPolicy() const { return m_policy; };

protected:
    CachePolicy & setId( int id )
        { m_id = id; return *this; };

    CachePolicy & setCachePolicy( const QString & policy )
        { m_policy = policy; return *this; };

private:
    int m_id;
    QString m_policy;

    friend class DataStore;
};

/***************************************************************************
 *   ItemMetaData                                                          *
 ***************************************************************************/
class ItemMetaData
{
public:
    ItemMetaData();
    ItemMetaData( int id, const QString & metadata, const MetaType & metatype );
    ~ItemMetaData();

    int getId() const { return m_id; };
    int getMetaTypeId() const { return m_metatype_id; };
    const QString & getItemMetaData() const { return m_metadata; };

protected:
    ItemMetaData & setId( int id )
        { m_id = id; return *this; };

    ItemMetaData & setMetaTypeId( int metatype_id )
        { m_metatype_id = metatype_id; return *this; };

    ItemMetaData & setItemMetaData( const QString & metadata)
        { m_metadata = metadata; return *this; };

private:
    int m_id;
    int m_metatype_id;
    QString m_metadata;

    friend class DataStore;
};

/***************************************************************************
 *   Location                                                              *
 ***************************************************************************/
class Location
{
public:
    Location();
    Location( int id, const QString & location,
              int policy_id, int resource_id );
    Location( int id, const QString & location,
              const CachePolicy & policy, const Resource & resource );
    ~Location();

    int getId() const { return m_id; };
    int getPolicyId() const { return m_policy_id; };
    int getResourceId() const { return m_resource_id; };
    const QString & getLocation() const { return m_location; };

protected:
    Location & setId( int id )
        { m_id = id; return *this; };

    Location & setPolicyId( int policy_id )
        { m_policy_id = policy_id; return *this; };

    Location & setResourceId( int resource_id )
        { m_resource_id = resource_id; return *this; };

    Location & setLocation( const QString & location )
        { m_location = location; return *this; };

private:
    int m_id;
    int m_policy_id;
    int m_resource_id;
    QString m_location;

    friend class DataStore;
};

QDebug & operator<< ( QDebug& d, const  Akonadi::Location& location);

/***************************************************************************
 *   MimeType                                                              *
 ***************************************************************************/
class MimeType
{
public:
    MimeType();
    MimeType( int id, const QString & mimetype );
    ~MimeType();

    int getId() const { return m_id; };
    const QString & getMimeType() const { return m_mimetype; };

protected:
    MimeType & setId( int id )
        { m_id = id; return *this; };

    MimeType & setMimeType( const QString & mimetype )
        { m_mimetype = mimetype; return *this; };

private:
    int m_id;
    QString m_mimetype;

    friend class DataStore;
};

/***************************************************************************
 *   MetaType                                                              *
 ***************************************************************************/
class MetaType
{
public:
    MetaType();
    MetaType( int id, const QString & metatype ,
              const MimeType & mimetype );
    ~MetaType();

    int getId() const { return m_id; };
    int getMimeTypeId() const { return m_mimetype_id; };
    const QString & getMetaType() const { return m_metatype; };

protected:
    MetaType & setId( int id )
        { m_id = id; return *this; };

    MetaType & setMimeTypeId( int mimetype_id )
        { m_mimetype_id = mimetype_id; return *this; };

    MetaType & setMetaType( const QString & metatype )
        { m_metatype = metatype; return *this; };

private:
    int m_id;
    int m_mimetype_id;
    QString m_metatype;

    friend class DataStore;
};

/***************************************************************************
 *   PimItem                                                               *
 ***************************************************************************/
class PimItem
{
public:
    PimItem();
    ~PimItem();

    int getId() const { return m_id; };
    int getMimeTypeId() const { return m_mimetype_id; };
    int getLocationId() const { return m_location_id; };
    const QByteArray & getData() const { return m_data; };

protected:
    PimItem & setId( int id )
        { m_id = id; return *this; };

    PimItem & setMimeTypeId( int mimetype_id )
        { m_mimetype_id = mimetype_id; return *this; };

    PimItem & setLocationId( int location_id )
        { m_location_id = location_id; return *this; };

    PimItem & setData( const QByteArray & data )
        { m_data = data; return *this; };

private:
    int m_id;
    int m_mimetype_id;
    int m_location_id;
    QByteArray m_data;

    friend class DataStore;
};

/***************************************************************************
 *   Resource                                                              *
 ***************************************************************************/
class Resource
{
public:
    Resource();
    Resource( int id, const QString & resource, int policy_id );
    Resource( int id, const QString & resource, const CachePolicy & policy );
    ~Resource();

    int getId() const { return m_id; };
    int getPolicyId() const { return m_policy_id; };
    const QString & getResource() const { return m_resource; };
    bool isValid() { return m_id != -1; }

protected:
    Resource & setId( int id )
        { m_id = id; return *this; };

    Resource & setPolicyId( int policy_id )
        { m_policy_id = policy_id; return *this; };

    Resource & setResource( const QString & resource )
        { m_resource = resource; return *this; };

private:
    int m_id;
    int m_policy_id;
    QString m_resource;

    friend class DataStore;
};

}


#endif
