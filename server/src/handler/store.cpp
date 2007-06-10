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

#include <QtCore/QStringList>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "handlerhelper.h"

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

  // ### Akonadi vs. IMAP conflict
  QList<PimItem> pimItems;
  if ( connection()->selectedLocation().id() == -1 || mStoreQuery.isUidStore() ) {
    pimItems = store->matchingPimItemsByUID( mStoreQuery.sequences() );
  } else {
//     if ( mStoreQuery.isUidStore() ) {
//       pimItems = store->matchingPimItemsByUID( mStoreQuery.sequences(), connection()->selectedLocation() );
//     } else {
      pimItems = store->matchingPimItemsBySequenceNumbers( mStoreQuery.sequences(), connection()->selectedLocation() );
//     }
  }

  qDebug() << "Store::commit()" << pimItems.count() << "items selected.";

  for ( int i = 0; i < pimItems.count(); ++i ) {
    switch ( mStoreQuery.dataType() ) {
      case StoreQuery::Flags:
        if ( mStoreQuery.operation() & StoreQuery::Replace ) {
          if ( !replaceFlags( pimItems[ i ], mStoreQuery.flags() ) )
            return failureResponse( "Unable to replace item flags." );
        } else if ( mStoreQuery.operation() & StoreQuery::Add ) {
          if ( !addFlags( pimItems[ i ], mStoreQuery.flags() ) )
            return failureResponse( "Unable to add item flags." );
        } else if ( mStoreQuery.operation() & StoreQuery::Delete ) {
          if ( !deleteFlags( pimItems[ i ], mStoreQuery.flags() ) )
            return failureResponse( "Unable to remove item flags." );
        }
        break;
      case StoreQuery::Data:
        if ( !store->updatePimItem( pimItems[ i ], mData ) )
          return failureResponse( "Unable to change item data." );
        break;
      case StoreQuery::Collection:
        if ( !store->updatePimItem( pimItems[ i ], HandlerHelper::collectionFromIdOrName( mStoreQuery.collection() ) ) )
          return failureResponse( "Unable to move item." );
        break;
      case StoreQuery::RemoteId:
        if ( !store->updatePimItem( pimItems[i], mStoreQuery.remoteId() ) )
          return failureResponse( "Unable to change remote id for item." );
        break;
      case StoreQuery::Dirty:
        PimItem item = pimItems.at( i );
        item.setDirty( false );
        if ( !item.update() )
          return failureResponse( "Unable to update item" );
        break;
    }

    if ( !( mStoreQuery.operation() & StoreQuery::Silent ) ) {
      QList<Flag> flags = pimItems[ i ].flags();
      QStringList flagList;
      for ( int j = 0; j < flags.count(); ++j )
        flagList.append( flags[ j ].name() );

      int itemPosition = store->pimItemPosition( pimItems[ i ] );
      response.setUntagged();
      response.setString( QByteArray::number( itemPosition ) + " FETCH (FLAGS (" + flagList.join( QLatin1String(" ") ).toUtf8() + "))" );
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


bool Store::replaceFlags( const PimItem &item, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = Flag::retrieveByName( QString::fromUtf8( flags[ i ] ) );
    if ( !flag.isValid() ) {
       // If the flag does not exist we'll create it now.
      if ( !store->appendFlag( QString::fromUtf8( flags[ i ] ) ) ) {
        qDebug( "Store::replaceFlags: Unable to add new flag '%s'", flags[ i ].data() );
        return false;
      } else {
        flag = Flag::retrieveByName( QString::fromUtf8( flags[ i ] ) );
        if ( !flag.isValid() )
          return false;
        else
          flagList.append( flag );
      }
    } else {
      flagList.append( flag );
    }
  }

  if ( !store->setItemFlags( item, flagList ) ) {
    qDebug( "Store::replaceFlags: Unable to set new item flags" );
    return false;
  }
  return true;
}

bool Store::addFlags( const PimItem &item, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = Flag::retrieveByName( QString::fromUtf8( flags[ i ] ) );
    if ( !flag.isValid() ) {
       // If the flag does not exist we'll create it now.
      if ( !store->appendFlag( QString::fromUtf8( flags[ i ]  ) ) ) {
        qDebug( "Store::addFlags: Unable to add new flag '%s'", flags[ i ].data() );
        return false;
      } else {
        flag = Flag::retrieveByName( QString::fromUtf8( flags[ i ] ) );
        if ( !flag.isValid() )
          return false;
        else
          flagList.append( flag );
      }
    } else {
      flagList.append( flag );
    }
  }

  if ( !store->appendItemFlags( item, flagList ) ) {
    qDebug( "Store::addFlags: Unable to add new item flags" );
    return false;
  }
  return true;
}

bool Store::deleteFlags( const PimItem &item, const QList<QByteArray> &flags )
{
  DataStore *store = connection()->storageBackend();

  QList<Flag> flagList;
  for ( int i = 0; i < flags.count(); ++i ) {
    Flag flag = Flag::retrieveByName( QString::fromUtf8( flags[ i ] ) );
    if ( !flag.isValid() )
      continue;

    flagList.append( flag );
  }

  if ( !store->removeItemFlags( item, flagList ) ) {
    qDebug( "Store::deleteFlags: Unable to add new item flags" );
    return false;
  }
  return true;
}

bool Akonadi::Store::inContinuation() const
{
  return mSize > -1;
}

bool Akonadi::Store::handleContinuation(const QByteArray & line)
{
  if ( line.size() > mSize )
    mData += line.left( mSize );
  else
    mData += line;
  mSize = qMax( mSize - line.size(), 0 );
  if ( !allDataRead() )
    return false;
  return commit();
}

bool Akonadi::Store::allDataRead() const
{
  return ( mSize == 0 );
}
