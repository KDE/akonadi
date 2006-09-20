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
#include "storage/datastore.h"
#include "storage/transaction.h"

#include "store.h"

using namespace Akonadi;

Store::Store()
  : Handler(), mSize( -1 )
{
}

Store::~Store()
{
}

bool Store::handleLine( const QByteArray& line )
{
  if ( inContinuation() )
    return handleContinuation( line );

  int start = line.indexOf( ' ' ) + 1; // skip tag

  if ( !mStoreQuery.parse( line.mid( start ) ) ) {
    Response response;
    response.setTag( tag() );
    response.setError();
    response.setString( "Syntax error" );

    emit responseAvailable( response );
    deleteLater();

    return true;
  }

  if ( mStoreQuery.continuationSize() > 0 ) {
    mSize = mStoreQuery.continuationSize();
    return startContinuation();
  }

  return commit();
}

bool Store::commit()
{
  mStoreQuery.dump();

  Response response;
  response.setUntagged();

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  QList<PimItem> pimItems;
  if ( connection()->selectedLocation().id() == -1 ) {
    pimItems = store->matchingPimItemsByUID( mStoreQuery.sequences() );
  } else {
    if ( mStoreQuery.isUidStore() ) {
      pimItems = store->matchingPimItemsByUID( mStoreQuery.sequences(), connection()->selectedLocation() );
    } else {
      pimItems = store->matchingPimItemsBySequenceNumbers( mStoreQuery.sequences(), connection()->selectedLocation() );
    }
  }

  for ( int i = 0; i < pimItems.count(); ++i ) {
    if ( mStoreQuery.dataType() == StoreQuery::Flags ) {
      if ( mStoreQuery.operation() & StoreQuery::Replace ) {
        replaceFlags( pimItems[ i ], mStoreQuery.flags() );
      } else if ( mStoreQuery.operation() & StoreQuery::Add ) {
        addFlags( pimItems[ i ], mStoreQuery.flags() );
      } else if ( mStoreQuery.operation() & StoreQuery::Delete ) {
        deleteFlags( pimItems[ i ], mStoreQuery.flags() );
      }
    } else if ( mStoreQuery.dataType() == StoreQuery::Data ) {
      store->updatePimItem( pimItems[ i ], mData );
    }

    if ( !( mStoreQuery.operation() & StoreQuery::Silent ) ) {
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

  if ( !transaction.commit() )
    return failureResponse( "Cannot commit transaction." );

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

bool Akonadi::Store::inContinuation() const
{
  return mSize > -1;
}

bool Akonadi::Store::handleContinuation(const QByteArray & line)
{
  mData += line;
  mSize -= line.size();
  if ( !allDataRead() )
    return false;
  return commit();
}

bool Akonadi::Store::allDataRead() const
{
  return ( mSize == 0 );
}
