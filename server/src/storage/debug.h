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

#ifndef DEBUG_H
#define DEBUG_H

#include "entity.h"

#include <QList>

namespace Akonadi {

/***************************************************************************
 *   CachePolicy                                                           *
 ***************************************************************************/
void debugCachePolicy( const CachePolicy & policy );
void debugCachePolicyList( const QList<CachePolicy> & list );

QDebug & operator<< ( QDebug& d, const  Akonadi::CachePolicy& policy );




/***************************************************************************
 *   MimeType                                                              *
 ***************************************************************************/
void debugMimeType( const MimeType & mimetype );
void debugMimeTypeList( const QList<MimeType> & list );

/***************************************************************************
 *   MetaType                                                              *
 ***************************************************************************/

/***************************************************************************
 *   PimItem                                                               *
 ***************************************************************************/

/***************************************************************************
 *   Resource                                                              *
 ***************************************************************************/
void debugResource( const Resource & resource );
void debugResourceList( const QList<Resource> & list );

}

#endif
