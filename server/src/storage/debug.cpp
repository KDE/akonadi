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

#include "debug.h"

#include <QtDebug>

using namespace Akonadi;

/***************************************************************************
 *   CachePolicy                                                           *
 ***************************************************************************/
void Akonadi::debugCachePolicy( const CachePolicy & policy )
{
  qDebug() << "CachePolicy => ID: " << policy.id()
           << " Policy: " << policy.policy();
}

void Akonadi::debugCachePolicyList( const QList<CachePolicy> & list )
{
  if ( list.empty() )
    qDebug() << "The list of CachePolicies is empty.";
  else
    foreach( CachePolicy policy, list )
      debugCachePolicy( policy );
}

QDebug & Akonadi::operator<< ( QDebug& d, const  Akonadi::CachePolicy& policy )
{
   d << "CachePolicy => ID: " << policy.id() 
     << " Policy: " << policy.policy() << endl;
   return d;
}

/***************************************************************************
 *   Flag                                                                  *
 ***************************************************************************/
void Akonadi::debugFlag( const Flag & flag )
{
  qDebug() << "Flag => ID: " << flag.id()
           << " Name: " << flag.name();
}

void Akonadi::debugFlagList( const QList<Flag> & list )
{
  if ( list.empty() )
    qDebug() << "The list of Flags is empty.";
  else
    foreach( Flag flag, list )
      debugFlag( flag );
}

/***************************************************************************
 *   MimeType                                                              *
 ***************************************************************************/
void Akonadi::debugMimeType( const MimeType & mimetype )
{
  qDebug() << "MimeType => ID: " << mimetype.id()
           << " MimeType: " << mimetype.mimeType();
}

void Akonadi::debugMimeTypeList( const QList<MimeType> & list )
{
  if ( list.empty() )
    qDebug() << "The list of MimeTypes is empty.";
  else
    foreach( MimeType mimetype, list )
      debugMimeType( mimetype );
}

/***************************************************************************
 *   MetaType                                                              *
 ***************************************************************************/

/***************************************************************************
 *   PimItem                                                               *
 ***************************************************************************/

/***************************************************************************
 *   Resource                                                              *
 ***************************************************************************/
void Akonadi::debugResource( const Resource & resource )
{
  qDebug() << "Resource => ID: " << resource.id()
           << " Resource: " << resource.resource()
           << " Policy-ID: " << resource.policyId();
}

void Akonadi::debugResourceList( const QList<Resource> & list )
{
  if ( list.empty() )
    qDebug() << "The list of Resources is empty.";
  else
    foreach( Resource resource, list )
      debugResource( resource );
}
