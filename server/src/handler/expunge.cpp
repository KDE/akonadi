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
#include "storage/datastore.h"

#include "expunge.h"

using namespace Akonadi;

Expunge::Expunge()
  : Handler()
{
}

Expunge::~Expunge()
{
}

bool Expunge::handleLine( const QByteArray& )
{
  Response response;

  Location location = connection()->selectedLocation();
  DataStore *store = connection()->storageBackend();

  Flag flag = store->flagByName( "\\Deleted" );
  if ( !flag.isValid() ) {
    response.setError();
    response.setString( "\\Deleted flag unknown" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  QList<PimItem> items = store->listPimItems( location, flag );
  for ( int i = 0; i < items.count(); ++i ) {
    const int position = store->pimItemPosition( items[ i ] );

    if ( store->cleanupPimItem( items[ i ] ) ) {
      response.setUntagged();
      response.setString( QByteArray::number( position ) + " EXPUNGE" );

      emit responseAvailable( response );
    } else {
      response.setTag( tag() );
      response.setError();
      response.setString( "internal error" );

      emit responseAvailable( response );
      deleteLater();
      return true;
    }
  }

  response.setTag( tag() );
  response.setSuccess();
  response.setString( "EXPUNGE completed" );

  emit responseAvailable( response );
  deleteLater();

  return true;
}
