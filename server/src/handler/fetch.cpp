/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "fetch.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "fetchhelper.h"
#include "response.h"

using namespace Akonadi;

Fetch::Fetch( Scope::SelectionScope scope )
  : mScope( scope )
{
}

bool Fetch::parseStream()
{
  // sequence set
  mScope.parseScope( m_streamParser );

  FetchHelper fetchHelper( mScope );
  fetchHelper.setConnection( connection() );
  fetchHelper.setStreamParser( m_streamParser );
  connect( &fetchHelper, SIGNAL( responseAvailable( const Response& ) ),
           this, SIGNAL( responseAvailable( const Response& ) ) );
  connect( &fetchHelper, SIGNAL( failureResponse( const QString& ) ),
           this, SLOT( slotFailureResponse( const QString& ) ) );

  if ( !fetchHelper.parseStream( "FETCH" ) )
    return false;

  if ( mScope.scope() == Scope::Uid )
    successResponse( "UID FETCH completed" );
  else if ( mScope.scope() == Scope::Rid )
    successResponse( "RID FETCH completed" );
  else
    successResponse( "FETCH completed" );

  return true;
}

void Fetch::slotFailureResponse( const QString &msg )
{
  failureResponse( msg );
}

