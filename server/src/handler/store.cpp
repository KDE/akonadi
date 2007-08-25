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

#include "store.h"

#include <QtCore/QStringList>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "handlerhelper.h"
#include "imapparser.h"
#include "storage/selectquerybuilder.h"

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
  int pos = line.indexOf( ' ' ) + 1; // skip tag
  QByteArray buffer;
  pos = ImapParser::parseString( line, buffer, pos );
  bool uidStore = false;
  if ( buffer == "UID" ) {
    uidStore = true;
    pos = ImapParser::parseString( line, buffer, pos ); // skip 'STORE'
  }

  Response response;
  response.setUntagged();

  QList<QByteArray> sequences;
  pos = ImapParser::parseString( line, buffer, pos );
  sequences = buffer.split( ',' );
  if ( sequences.isEmpty() )
    return failureResponse( "No item specified" );

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  // ### Akonadi vs. IMAP conflict
  QList<PimItem> pimItems;
  if ( connection()->selectedLocation().id() == -1 || uidStore ) {
    pimItems = store->matchingPimItemsByUID( sequences );
  } else {
//     if ( mStoreQuery.isUidStore() ) {
//       pimItems = store->matchingPimItemsByUID( mStoreQuery.sequences(), connection()->selectedLocation() );
//     } else {
      pimItems = store->matchingPimItemsBySequenceNumbers( sequences, connection()->selectedLocation() );
//     }
  }

  qDebug() << "Store::commit()" << pimItems.count() << "items selected.";

  // parse command
  QByteArray command;
  pos = ImapParser::parseString( line, command, pos );
  Operation op = Replace;
  bool silent = false;

  if ( command.startsWith( '+' ) ) {
    op = Add;
    command = command.mid( 1 );
  } else if ( command.startsWith( '-' ) ) {
    op = Delete;
    command = command.mid( 1 );
  }
  if ( command.endsWith( ".SILENT" ) ) {
    silent = true;
    command.chop( 7 );
  }

  for ( int i = 0; i < pimItems.count(); ++i ) {
    if ( command == "FLAGS" ) {
      QList<QByteArray> flags;
      pos = ImapParser::parseParenthesizedList( line, flags, pos );
      if ( op == Replace ) {
        if ( !replaceFlags( pimItems[ i ], flags ) )
          return failureResponse( "Unable to replace item flags." );
      } else if ( op == Add ) {
        if ( !addFlags( pimItems[ i ], flags ) )
          return failureResponse( "Unable to add item flags." );
      } else if ( op == Delete ) {
        if ( !deleteFlags( pimItems[ i ], flags ) )
          return failureResponse( "Unable to remove item flags." );
      }
    } else if ( command == "RFC822" ) {
      pos = ImapParser::parseString( line, buffer, pos );
      if ( !store->updatePimItem( pimItems[ i ], buffer ) )
        return failureResponse( "Unable to change item data." );
    } else if ( command == "COLLECTION" ) {
      pos = ImapParser::parseString( line, buffer, pos );
      if ( !store->updatePimItem( pimItems[ i ], HandlerHelper::collectionFromIdOrName( buffer ) ) )
        return failureResponse( "Unable to move item." );
    } else if ( command == "REMOTEID" ) {
      pos = ImapParser::parseString( line, buffer, pos );
      if ( !store->updatePimItem( pimItems[i], QString::fromUtf8( buffer ) ) )
        return failureResponse( "Unable to change remote id for item." );
    } else if ( command == "DIRTY" ) {
        PimItem item = pimItems.at( i );
        item.setDirty( false );
        if ( !item.update() )
          return failureResponse( "Unable to update item" );
    } else {
      pos = ImapParser::parseString( line, buffer, pos );
      qDebug() << "Multipart store: IMPLEMENT ME!" << command << buffer;
      Part part;
      SelectQueryBuilder<Part> qb;
      qb.addValueCondition( Part::pimItemIdColumn(), "=", pimItems[ i ].id() );
      qb.addValueCondition( Part::nameColumn(), "=", QString::fromUtf8( command ) );
      if ( !qb.exec() )
        return failureResponse( "Unable to check item part existence" );
      Part::List result = qb.result();
      if ( !result.isEmpty() )
        part = result.first();

      part.setData( buffer );
      part.setName( QString::fromUtf8( command ) );
      part.setPimItemId( pimItems[ i ].id() );
      if ( part.isValid() ) {
        if ( !part.update() )
          return failureResponse( "Unable to update item part" );
      } else {
        if ( !part.insert() )
          return failureResponse( "Unable to add item part" );
      }
    }

    if ( !silent ) {
      QList<Flag> flags = pimItems[ i ].flags();
      QStringList flagList;
      for ( int j = 0; j < flags.count(); ++j )
        flagList.append( flags[ j ].name() );

      response.setUntagged();
      // IMAP protocol violation: should actually be the sequence number
      response.setString( QByteArray::number( pimItems[i].id() ) + " FETCH (FLAGS (" + flagList.join( QLatin1String(" ") ).toUtf8() + "))" );
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
