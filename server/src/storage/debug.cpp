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
void debugCachePolicy( const CachePolicy & policy )
{
  qDebug() << "CachePolicy => ID: " << policy.getId()
           << " Policy: " << policy.getPolicy();
}

void debugCachePolicyList( const QList<CachePolicy> & list )
{
  if ( list.empty() )
    qDebug() << "The list of CachePolicies is empty.";
  else
    foreach( CachePolicy policy, list )
      debugCachePolicy( policy );
}

/***************************************************************************
 *   MimeType                                                              *
 ***************************************************************************/
void debugMimeType( const MimeType & mimetype )
{
  qDebug() << "MimeType => ID: " << mimetype.getId()
           << " MimeType: " << mimetype.getMimeType();
}

void debugMimeTypeList( const QList<MimeType> & list )
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
void debugResource( const Resource & resource )
{
  qDebug() << "Resource => ID: " << resource.getId()
           << " Resource: " << resource.getResource()
           << " Policy-ID: " << resource.getPolicyId();
}

void debugResourceList( const QList<Resource> & list )
{
  if ( list.empty() )
    qDebug() << "The list of Resources is empty.";
  else
    foreach( Resource resource, list )
      debugResource( resource );
}
