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

/***************************************************************************
 *   CachePolicy                                                           *
 ***************************************************************************/
CachePolicy::CachePolicy()
{}

CachePolicy::CachePolicy( int id, const QString & policy )
  : Entity( id ), m_policy( policy )
{}

CachePolicy::~CachePolicy()
{}

/***************************************************************************
 *   Flag                                                                  *
 ***************************************************************************/
Flag::Flag()
{}

Flag::Flag( int id, const QString & name )
  : Entity( id ), m_name( name )
{}

Flag::~Flag()
{}

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
    m_flags = "FLAGS (Answered)";
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
{}

QDebug & operator<< ( QDebug& d, const  Akonadi::Location& location)
{
   d << location.location();

   return d;
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

/***************************************************************************
 *   MetaType                                                              *
 ***************************************************************************/
MetaType::MetaType()
{
}

MetaType::~MetaType()
{
}

/***************************************************************************
 *   PimItem                                                               *
 ***************************************************************************/
PimItem::PimItem()
{
}

PimItem::PimItem( int id, const QByteArray & data, int location_id,
                  int mimetype_id )
    : Entity( id ), m_data( data ), m_location_id( location_id ),
      m_mimetype_id( mimetype_id )
{}


PimItem::~PimItem()
{
}

/***************************************************************************
 *   Resource                                                              *
 ***************************************************************************/
Resource::Resource()
{
}

Resource::Resource( int id, const QString & resource, int policy_id )
    : Entity( id ), m_policy_id( policy_id ), m_resource( resource )
{}

Resource::Resource( int id, const QString & resource, const CachePolicy & policy )
    : Entity( id ), m_policy_id( policy.id() ), m_resource( resource )
{}

Resource::~Resource()
{}

}

QByteArray Akonadi::MimeType::asCommaSeparatedString( const QList<MimeType> &types )
{
    QStringList list;
    foreach ( MimeType mt, types )
    {
        list.append( mt.mimeType() );
    }
    return list.join( "," ).toLatin1();
}

// namespace Akonadi

