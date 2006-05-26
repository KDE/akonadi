/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                        *
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

#include "response.h"
#include "search.h"
#include "searchprovidermanager.h"

using namespace Akonadi;

/**
 * Splits a line at white spaces which are not escaped by '"'
 */
static QList<QByteArray> splitLine( const QByteArray &line )
{
  QList<QByteArray> retval;

  int i, start = 0;
  bool escaped = false;
  for ( i = 0; i < line.count(); ++i ) {
    if ( line[ i ] == ' ' ) {
      if ( !escaped ) {
        retval.append( line.mid( start, i - start ) );
        start = i + 1;
      }
    } else if ( line[ i ] == '"' ) {
      if ( escaped ) {
        escaped = false;
      } else {
        escaped = true;
      }
    }
  }

  retval.append( line.mid( start, i - start ) );

  return retval;
}

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

    QList<QByteArray> junks = splitLine( line );

    /**
     * A search can have the following forms:
     *
     * 123 SEARCH CHARSET <charset> MIMETYPE <mimetype> ...
     * 123 SEARCH MIMETYPE <mimetype> ...
     * 123 SEARCH ...
     *
     * so we first have to find out which case matches.
     */
    QByteArray mimeType;
    if ( junks[ 2 ].toUpper() == "CHARSET" ) {
      if ( junks[ 4 ].toUpper() == "MIMETYPE" )
        mimeType = junks[ 5 ].toLower();
    } else {
      if ( junks[ 2 ].toUpper() == "MIMETYPE" )
        mimeType = junks[ 3 ].toLower();
    }

    /**
     * If no mimetype is specified fallback to normal IMAP search.
     */
    if ( mimeType.isEmpty() )
      mimeType = "message/rfc822";

    SearchProvider *provider = 0;
    provider = SearchProviderManager::self()->createSearchProviderForMimetype( mimeType, connection() );

    if ( provider ) {
      QList<QByteArray> uids;
      bool ok = true;
      try {
        uids = provider->queryForUids( junks );
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
