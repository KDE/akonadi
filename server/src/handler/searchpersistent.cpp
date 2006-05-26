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
#include "response.h"
#include "persistentsearch.h"
#include "persistentsearchmanager.h"
#include "searchhelper.h"
#include "searchprovidermanager.h"

#include "searchpersistent.h"

using namespace Akonadi;

SearchPersistent::SearchPersistent()
  : Handler()
{
}

SearchPersistent::~SearchPersistent()
{
}

bool SearchPersistent::handleLine( const QByteArray& line )
{
    Response response;
    response.setUntagged();

    QList<QByteArray> junks = SearchHelper::splitLine( line );

    /**
     * A persistent search can have the following forms:
     *
     *    123 SEARCH_STORE <name> <criterion1> <criterion2> ...
     * or
     *    123 SEARCH_DELETE <name>
     *
     * so we first have to find out which case matches.
     */
    if ( junks[ 1 ].toUpper() == "SEARCH_STORE" ) {
      if ( junks.count() < 4 ) {
        response.setTag( tag() );
        response.setError();
        response.setString( "Too less arguments" );
        emit responseAvailable( response );
      } else {
        QByteArray mimeType = SearchHelper::extractMimetype( junks, 3 );

        SearchProvider *provider = SearchProviderManager::self()->createSearchProviderForMimetype( mimeType, connection() );
        PersistentSearch *persistentSearch = new PersistentSearch( junks.mid( 3 ), provider );

        QString identifier = QString::fromUtf8( junks[ 2 ] );
        PersistentSearchManager::self()->addPersistentSearch( identifier, persistentSearch );

        response.setTag( tag() );
        response.setSuccess();
        response.setString( "SEARCH_STORE completed" );
        emit responseAvailable( response );
      }
    } else if ( junks[ 1 ].toUpper() == "SEARCH_DELETE" ) {
      if ( junks.count() < 3 ) {
        response.setTag( tag() );
        response.setError();
        response.setString( "Too less arguments" );
        emit responseAvailable( response );
      } else {
        QString identifier = QString::fromUtf8( junks[ 2 ] );
        PersistentSearchManager::self()->removePersistentSearch( identifier );

        response.setTag( tag() );
        response.setSuccess();
        response.setString( "SEARCH_DELETE completed" );
        emit responseAvailable( response );
      }
    } else if ( junks[ 1 ].toUpper() == "SEARCH_DEBUG" ) {
      CollectionList collections = PersistentSearchManager::self()->collections();
      for ( int i = 0; i < collections.count(); ++i ) {
        response.setString( collections[ i ].identifier().toLatin1() );
        emit responseAvailable( response );
      }

      response.setTag( tag() );
      response.setSuccess();
      response.setString( "SEARCH_DEBUG completed" );
      emit responseAvailable( response );
    }

    deleteLater();

    return true;
}
