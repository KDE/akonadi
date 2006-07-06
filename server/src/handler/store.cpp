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

#include <QStringList>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "storequery.h"
#include "storage/datastore.h"

#include "store.h"

using namespace Akonadi;

Store::Store()
  : Handler()
{
}

Store::~Store()
{
}

bool Store::handleLine( const QByteArray& line )
{
  Response response;
  response.setUntagged();

  int start = line.indexOf( ' ' ) + 1; // skip tag

  StoreQuery storeQuery;

  if ( !storeQuery.parse( line.mid( start ) ) ) {
    response.setTag( tag() );
    response.setError();
    response.setString( "Syntax error" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  storeQuery.dump();

  DataStore *store = connection()->storageBackend();

  QList<PimItem> pimItems;
  if ( connection()->selectedLocation().id() == -1 ) {
    pimItems = store->matchingPimItems( storeQuery.sequences() );
  } else {
    if ( storeQuery.isUidStore() ) {
      pimItems = store->matchingPimItems( storeQuery.sequences() );
    } else {
      pimItems = store->matchingPimItemsByLocation( storeQuery.sequences(), connection()->selectedLocation() );
    }
  }

  for ( int i = 0; i < pimItems.count(); ++i ) {
    if ( storeQuery.type() & StoreQuery::Replace ) {
      replaceFlags( pimItems[ i ], storeQuery.flags() );
    } else if ( storeQuery.type() & StoreQuery::Add ) {
      addFlags( pimItems[ i ], storeQuery.flags() );
    } else if ( storeQuery.type() & StoreQuery::Delete ) {
      deleteFlags( pimItems[ i ], storeQuery.flags() );
    }

    if ( !( storeQuery.type() & StoreQuery::Silent ) ) {
      QList<Flag> flags = store->itemFlags( pimItems[ i ] );
      QStringList flagList;
      for ( int j = 0; j < flags.count(); ++j )
        flagList.append( flags[ j ].name() );

      int itemPosition = store->pimItemPosition( pimItems[ i ] );
      response.setUntagged();
      response.setString( QByteArray::number( itemPosition ) + " FETCH (FLAGS (" + flagList.join( " " ).toLatin1() + "))" );
      emit responseAvailable( response );
    }
  }

  response.setTag( tag() );
  response.setSuccess();
  response.setString( "STORE completed" );

  emit responseAvailable( response );
  deleteLater();

  return true;
}


void Store::replaceFlags( const PimItem &item, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = store->flagByName( flags[ i ] );
    if ( !flag.isValid() ) {
      /**
       * If the flag does not exist we'll create it now.
       */
      if ( !store->appendFlag( flags[ i ] ) ) {
        qDebug( "Store::replaceFlags: Unable to add new flag '%s'", flags[ i ].data() );
        return;
      } else {
        flag = store->flagByName( flags[ i ] );
        if ( !flag.isValid() )
          return;
        else
          flagList.append( flag );
      }
    } else {
      flagList.append( flag );
    }
  }

  if ( !store->setItemFlags( item, flagList ) ) {
    qDebug( "Store::replaceFlags: Unable to set new item flags" );
    return;
  }
}

void Store::addFlags( const PimItem &item, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = store->flagByName( flags[ i ] );
    if ( !flag.isValid() ) {
      /**
       * If the flag does not exist we'll create it now.
       */
      if ( !store->appendFlag( flags[ i ] ) ) {
        qDebug( "Store::addFlags: Unable to add new flag '%s'", flags[ i ].data() );
        return;
      } else {
        flag = store->flagByName( flags[ i ] );
        if ( !flag.isValid() )
          return;
        else
          flagList.append( flag );
      }
    } else {
      flagList.append( flag );
    }
  }

  if ( !store->appendItemFlags( item, flagList ) ) {
    qDebug( "Store::addFlags: Unable to add new item flags" );
    return;
  }
}

void Store::deleteFlags( const PimItem &item, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = store->flagByName( flags[ i ] );
    if ( !flag.isValid() )
      continue;

    flagList.append( flag );
  }

  if ( !store->removeItemFlags( item, flagList ) ) {
    qDebug( "Store::deleteFlags: Unable to add new item flags" );
    return;
  }
}
