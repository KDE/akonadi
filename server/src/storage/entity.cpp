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

#include <QStringList>
#include <QSqlQuery>
#include <QVariant>
#include "entity.h"

namespace Akonadi {

Entity::Entity()
  : m_id( -1 )
{
}

Entity::Entity( int id )
  : m_id( id )
{
}

int Entity::id() const
{
  return m_id;
}

void Entity::setId( int id )
{
  m_id = id;
}

bool Entity::isValid() const
{
  return m_id != -1;
}

/***************************************************************************
 *   CachePolicy                                                           *
 ***************************************************************************/
CachePolicy::CachePolicy()
{
}

CachePolicy::CachePolicy( int id, const QString & policy )
  : Entity( id ), m_policy( policy )
{
}

CachePolicy::~CachePolicy()
{
}

const QString & CachePolicy::policy() const
{
  return m_policy;
}

void CachePolicy::setCachePolicy( const QString & policy )
{
  m_policy = policy;
}

/***************************************************************************
 *   Flag                                                                  *
 ***************************************************************************/
Flag::Flag()
{
}

Flag::Flag( int id, const QString & name )
  : Entity( id ), m_name( name )
{
}

Flag::~Flag()
{
}

const QString & Flag::name() const
{
  return m_name;
}

void Flag::setName( const QString & name )
{
  m_name = name;
}

/***************************************************************************
 *   ItemMetaData                                                          *
 ***************************************************************************/

ItemMetaData::ItemMetaData()
{
}

ItemMetaData::ItemMetaData( int id, const QString & metadata, const MetaType & metatype )
  : Entity( id ), m_metatype_id( metatype.id() ), m_metadata( metadata )
{
}

ItemMetaData::~ItemMetaData()
{
}

int ItemMetaData::metaTypeId() const
{
  return m_metatype_id;
}

const QString & ItemMetaData::itemMetaData() const
{
  return m_metadata;
}

void ItemMetaData::setMetaTypeId( int metatype_id )
{
  m_metatype_id = metatype_id;
}

void ItemMetaData::setItemMetaData( const QString & metadata )
{
  m_metadata = metadata;
}

/***************************************************************************
 *   MimeType                                                              *
 ***************************************************************************/
MimeType::MimeType()
{}

MimeType::MimeType( int id, const QString & mimetype )
  : Entity( id ), m_mimetype( mimetype )
{}

MimeType::~MimeType()
{
}

QString MimeType::mimeType() const
{
  return m_mimetype;
}

QByteArray Akonadi::MimeType::asCommaSeparatedString( const QList<MimeType> &types, const QByteArray &separator )
{
  QStringList list;
  foreach ( MimeType mt, types )
    list.append( mt.mimeType() );

  return list.join( QString::fromLatin1( separator ) ).toLatin1();
}

void MimeType::setMimeType( const QString & mimetype )
{
  m_mimetype = mimetype;
}

/***************************************************************************
 *   Location                                                              *
 ***************************************************************************/
Location::Location()
{
    init();
}

Location::Location( int id, const QString & location,
                    int policy_id, int resource_id )
  : Entity( id ), m_policy_id( policy_id ),
    m_resource_id( resource_id ), m_location( location )
{
    init();
}

Location::Location( int id, const QString & location,
                    const CachePolicy & policy, const Resource & resource )
    : Entity( id ), m_policy_id( policy.id() ),
    m_resource_id( resource.id() ), m_location( location )
{
    init();
}

void Location::init()
{
    // FIXME, obviously
    m_flags = QLatin1String("FLAGS (Answered)");
}

void Location::fillFromQuery( const QSqlQuery & query )
{
    m_id = query.value(0).toInt();
    m_location = query.value(1).toString();
    m_policy_id = query.value(2).toInt();
    m_resource_id = query.value(3).toInt();
    m_exists = query.value(4).toInt();
    m_recent = query.value(5).toInt();
    m_unseen = query.value(6).toInt();
    m_firstUnseen = query.value(7).toInt();
    m_uidValidity = query.value(8).toInt();
    // FIXME flags
}

Location::~Location()
{
}

int Location::policyId() const
{
  return m_policy_id;
}

int Location::resourceId() const
{
  return m_resource_id;
}

const QString & Location::location() const
{
  return m_location;
}

void Location::setMimeTypes( const QList<MimeType> & types )
{
  m_mimeTypes = types;
}

QString Location::flags() const
{
  return m_flags;
}

int Location::exists() const
{
  return m_exists;
}

int Location::recent() const
{
  return m_recent;
}

int Location::unseen() const
{
  return m_unseen;
}

int Location::firstUnseen() const
{
  return m_unseen;
}

long Location::uidValidity() const
{
  return m_uidValidity;
}

QList<MimeType> Location::mimeTypes() const
{
  return m_mimeTypes;
}

void Location::setPolicyId( int policy_id )
{
  m_policy_id = policy_id;
}

void Location::setResourceId( int resource_id )
{
  m_resource_id = resource_id;
}

void Location::setLocation( const QString & location )
{
  m_location = location;
}

QDebug & operator<<( QDebug& d, const  Akonadi::Location& location )
{
   d << location.location();

   return d;
}


/***************************************************************************
 *   MetaType                                                              *
 ***************************************************************************/
MetaType::MetaType()
{
}

MetaType::MetaType( int id, const QString & metatype, const MimeType & mimetype )
  : Entity( id ), m_mimetype_id( mimetype.id() ), m_metatype( metatype )
{
}

MetaType::~MetaType()
{
}

int MetaType::mimeTypeId() const
{
  return m_mimetype_id;
}

const QString & MetaType::metaType() const
{
  return m_metatype;
}

void MetaType::setMimeTypeId( int mimetype_id )
{
  m_mimetype_id = mimetype_id;
}

void MetaType::setMetaType( const QString & metatype )
{
  m_metatype = metatype;
}

/***************************************************************************
 *   PimItem                                                               *
 ***************************************************************************/
PimItem::PimItem()
{
}

PimItem::PimItem( int id, const QByteArray & remote_id,
                  const QByteArray & data, int location_id, int mimetype_id,
                  const QDateTime & dateTime )
    : Entity( id ), m_data( data ), m_location_id( location_id ),
      m_mimetype_id( mimetype_id ), m_remote_id( remote_id ),
      m_datetime( dateTime )
{
}

PimItem::~PimItem()
{
}

int PimItem::mimeTypeId() const
{
  return m_mimetype_id;
}

int PimItem::locationId() const
{
  return m_location_id;
}

QByteArray PimItem::remoteId() const
{
  return m_remote_id;
}

const QByteArray & PimItem::data() const
{
  return m_data;
}

QDateTime PimItem::dateTime() const
{
  return m_datetime;
}

void PimItem::setMimeTypeId( int mimetype_id )
{
  m_mimetype_id = mimetype_id;
}

void PimItem::setLocationId( int location_id )
{
  m_location_id = location_id;
}

void PimItem::setData( const QByteArray & data )
{
  m_data = data;
}

void PimItem::setRemoteId( const QByteArray & remote_id )
{
  m_remote_id = remote_id;
}

void PimItem::setDateTime( const QDateTime & dateTime )
{
  m_datetime = dateTime;
}

/***************************************************************************
 *   Resource                                                              *
 ***************************************************************************/
Resource::Resource()
{
}

Resource::Resource( int id, const QString & resource, int policy_id )
    : Entity( id ), m_policy_id( policy_id ), m_resource( resource )
{
}

Resource::Resource( int id, const QString & resource, const CachePolicy & policy )
    : Entity( id ), m_policy_id( policy.id() ), m_resource( resource )
{
}

Resource::~Resource()
{
}

int Resource::policyId() const
{
  return m_policy_id;
}

const QString & Resource::resource() const
{
  return m_resource;
}

void Resource::setPolicyId( int policy_id )
{
  m_policy_id = policy_id;
}

void Resource::setResource( const QString & resource )
{
  m_resource = resource;
}

}

// namespace Akonadi

