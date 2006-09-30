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
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QString>

class QSqlQuery;

namespace Akonadi {

class DataStore;

class CachePolicy;
class ItemMetaData;
class Location;
class MimeType;
class MetaType;
class PimItem;
class Resource;

class Entity
{
  public:
    int id() const;
    void setId( int id );

    bool isValid() const;

  protected:
    Entity();
    Entity( int id );

    int m_id;
};

/***************************************************************************
 *   CachePolicy                                                           *
 ***************************************************************************/
class CachePolicy : public Entity
{
  public:
    CachePolicy();
    CachePolicy( int id, const QString & policy );
    ~CachePolicy();

    const QString & policy() const;

  protected:
    void setCachePolicy( const QString & policy );

  private:
    QString m_policy;
    friend class DataStore;
};

/***************************************************************************
 *   Flag                                                                  *
 ***************************************************************************/
class Flag : public Entity
{
  public:
    Flag();
    Flag( int id, const QString & name );
    ~Flag();

    const QString & name() const;

  protected:
    void setName( const QString & name );

  private:
    QString m_name;
    friend class DataStore;
};

/***************************************************************************
 *   ItemMetaData                                                          *
 ***************************************************************************/
class ItemMetaData : public Entity
{
  public:
    ItemMetaData();
    ItemMetaData( int id, const QString & metadata, const MetaType & metatype );
    ~ItemMetaData();

    int metaTypeId() const;
    const QString & itemMetaData() const;

  protected:
    void setMetaTypeId( int metatype_id );
    void setItemMetaData( const QString & metadata );

  private:
    int m_metatype_id;
    QString m_metadata;

    friend class DataStore;
};

/***************************************************************************
 *   MimeType                                                              *
 ***************************************************************************/
class MimeType : public Entity
{
  public:
    MimeType();
    MimeType( int id, const QString & mimetype );
    ~MimeType();

    QString mimeType() const;
    static QByteArray asCommaSeparatedString( const QList<MimeType> &types, const QByteArray &separator = QByteArray(",") );

  protected:
    void setMimeType( const QString & mimetype );

  private:
    QString m_mimetype;

    friend class DataStore;
};

/***************************************************************************
 *   Location                                                              *
 ***************************************************************************/
class Location : public Entity
{
  public:
    Location();
    Location( int id, const QString & location,
              int policy_id, int resource_id );
    Location( int id, const QString & location,
              const CachePolicy & policy, const Resource & resource );
    ~Location();

    void fillFromQuery( const QSqlQuery & query );

    int policyId() const;
    int resourceId() const;
    const QString & location() const;
    void setMimeTypes( const QList<MimeType> & types );

    QString flags() const;
    int exists() const;
    int recent() const;
    int unseen() const;
    int firstUnseen() const;
    long uidValidity() const;
    QList<MimeType> mimeTypes() const;

  protected:
    void setPolicyId( int policy_id );
    void setResourceId( int resource_id );
    void setLocation( const QString & location );

    void init();

  private:
    int m_policy_id;
    int m_resource_id;
    QString m_location;
    int m_exists;
    int m_recent;
    int m_unseen;
    int m_firstUnseen;
    long m_uidValidity;
    QString m_flags;
    QList<MimeType> m_mimeTypes;

    friend class DataStore;
};

QDebug & operator<< ( QDebug& d, const  Akonadi::Location& location );

/***************************************************************************
 *   MetaType                                                              *
 ***************************************************************************/
class MetaType : public Entity
{
  public:
    MetaType();
    MetaType( int id, const QString & metatype ,
              const MimeType & mimetype );
    ~MetaType();

    int mimeTypeId() const;
    const QString & metaType() const;

  protected:
    void setMimeTypeId( int mimetype_id );
    void setMetaType( const QString & metatype );

  private:
    int m_mimetype_id;
    QString m_metatype;

    friend class DataStore;
};

/***************************************************************************
 *   Part                                                                  *
 ***************************************************************************/
// TODO

/***************************************************************************
 *   PartFlag                                                              *
 ***************************************************************************/
// TODO

/***************************************************************************
 *   PartMetaData                                                          *
 ***************************************************************************/
// TODO

/***************************************************************************
 *   PimItem                                                               *
 ***************************************************************************/
class PimItem : public Entity
{
  public:
    PimItem();
    PimItem( int id, const QByteArray & remote_id,
             const QByteArray & data, int location_id, int mimetype_id,
             const QDateTime & dateTime );
    ~PimItem();

    int mimeTypeId() const;
    int locationId() const;
    QByteArray remoteId() const;
    const QByteArray & data() const;
    QDateTime dateTime() const;

  protected:
    void setMimeTypeId( int mimetype_id );
    void setLocationId( int location_id );
    void setRemoteId( const QByteArray & remote_id );
    void setData( const QByteArray & data );
    void setDateTime( const QDateTime & dateTime );

  private:
    QByteArray m_data;
    int m_location_id;
    int m_mimetype_id;
    QByteArray m_remote_id;
    QDateTime m_datetime;

    friend class DataStore;
};

/***************************************************************************
 *   Resource                                                              *
 ***************************************************************************/
class Resource : public Entity
{
  public:
    Resource();
    Resource( int id, const QString & resource, int policy_id );
    Resource( int id, const QString & resource, const CachePolicy & policy );
    ~Resource();

    int policyId() const;
    const QString & resource() const;

  protected:
    void setPolicyId( int policy_id );
    void setResource( const QString & resource );

  private:
    int m_policy_id;
    QString m_resource;

    friend class DataStore;
};

/**
  Represents a persistent search aka virtual folder.
*/
class PersistentSearch : public Entity
{
  public:
    PersistentSearch();
    PersistentSearch( int id, const QString &name, const QByteArray &query );

    QString name() const { return m_name; }
    QByteArray query() const { return m_query; }

  private:
    QString m_name;
    QByteArray m_query;
};

}


#endif
