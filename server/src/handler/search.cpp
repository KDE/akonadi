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

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "searchhelper.h"
#include "searchprovidermanager.h"

#include "search.h"

using namespace Akonadi;

Search::Search()
  : Handler()
{
}

Search::~Search()
{
}

bool Search::handleLine( const QByteArray& line )
{
  Response response;
  response.setUntagged();

  QList<QByteArray> junks = SearchHelper::splitLine( line );

  if ( junks.count() < 4 ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Too few arguments" );
    emit responseAvailable( response );
    return false;
  }

  QByteArray mimeType = SearchHelper::extractMimetype( junks, 2 );
  if ( mimeType.isEmpty() ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Too few arguments" );
    emit responseAvailable( response );
    return false;
  }

  SearchProvider *provider = 0;
  provider = SearchProviderManager::self()->createSearchProviderForMimetype( mimeType );

  if ( provider ) {
    QList<QByteArray> uids;
    bool ok = true;
    try {
      uids = provider->queryForUids( junks, connection()->storageBackend() );
    } catch ( ... ) {
      ok = false;
    }

    if ( ok ) {
      response.setSuccess();

      QByteArray msg = "SEARCH";
      for ( int i = 0; i < uids.count(); ++i )
        msg += " " + uids[ i ];

      response.setString( msg );
      emit responseAvailable( response );

      response.setSuccess();
      response.setTag( tag() );
      response.setString( "SEARCH completed" );
      emit responseAvailable( response );

    } else {
      response.setTag( tag() );
      response.setError();
      response.setString( "Invalid search criteria" );

      emit responseAvailable( response );
    }
  } else {
    response.setTag( tag() );
    response.setError();
    response.setString( QByteArray( "No handler for MIMETYPE '" + mimeType + "' found" ) );

    emit responseAvailable( response );
  }

  deleteLater();

  return true;
}
