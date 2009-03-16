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

#include "akonadi.h"
#include "akonadiconnection.h"
#include "handlerhelper.h"
#include "response.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/itemqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/parthelper.h"
#include "storage/dbconfig.h"
#include "storage/itemretriever.h"

#include "libs/imapparser_p.h"
#include "imapstreamparser.h"

#include <QtCore/QStringList>
#include <QLocale>
#include <QDebug>

using namespace Akonadi;

Store::Store(bool isUid)
  : Handler()
  , mPos( 0 )
  , mPreviousRevision( -1 )
  , mSize( 0 )
  , mUidStore( isUid )
{
}

Store::~Store()
{
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
    qDebug( "Store::deleteFlags: Unable to remove item flags" );
    return false;
  }
  return true;
}

bool Store::parseStream()
{
  qDebug() << "Store::parseStream" ;
  parseCommandStream();
  QList<QByteArray> changes;
  changes = m_streamParser->readParenthesizedList();

  // check if this is a item move, and fetch all items in that case
  // TODO: strictly only needed for inter-resource moves
  bool isMove = false;
  for ( int i = 0; i < changes.size() - 1 && !isMove; i += 2 ) {
    QByteArray command = changes.at( i );
    if ( command.endsWith( ".SILENT" ) )
      command.chop( 7 );
    if ( command == "COLLECTION" )
      isMove = true;
  }
  if ( isMove ) {
    ItemRetriever retriever( connection() );
    retriever.setItemSet( mItemSet, mUidStore );
    retriever.setRetrieveFullPayload( true );
    retriever.exec();
  }

  DataStore *store = connection()->storageBackend();
  Transaction transaction( store );

  SelectQueryBuilder<PimItem> qb;
  ItemQueryHelper::itemSetToQuery( mItemSet, mUidStore, connection(), qb );
  if ( !qb.exec() )
    return failureResponse( "Unable to retrieve items" );
  QList<PimItem> pimItems = qb.result();
  if ( pimItems.isEmpty() )
    return failureResponse( "No items found" );

  // Set the same modification time for each item.
  QDateTime modificationtime = QDateTime::currentDateTime().toUTC();

  for ( int i = 0; i < pimItems.count(); ++i ) {
    if ( mPreviousRevision >= 0 ) {
      // check if revision number of given items and their database equivalents match
      if ( pimItems.at( i ).rev() != (int)mPreviousRevision ) {
        return failureResponse( "Item was modified elsewhere, aborting STORE." );
      }
    }

    // update item revision
    pimItems[ i ].setRev( pimItems[ i ].rev() + 1 );
    pimItems[ i ].setDatetime( modificationtime );
    if ( !pimItems[ i ].update() ) {
      return failureResponse( "Unable to update item revision" );
    }
  }

  for ( int i = 0; i < changes.size() - 1; i += 2 ) {
    // parse command
    QByteArray command = changes[ i ];
    Operation op = Replace;
    bool silent = false;
    if ( command.isEmpty() )
      break;

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
    const QByteArray value = changes[i + 1];

    for ( int i = 0; i < pimItems.count(); ++i ) {
      // handle command
      if ( command == "FLAGS" ) {
        QList<QByteArray> flags;
        ImapParser::parseParenthesizedList( value, flags );
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
      } else if ( command == "PARTS" ) {
        QList<QByteArray> parts;
        ImapParser::parseParenthesizedList( value, parts );
        if ( op == Delete ) {
          if ( !store->removeItemParts( pimItems[ i ], parts ) )
            return failureResponse( "Unable to remove item parts." );
        }
      } else if ( command == "COLLECTION" ) {
        if ( !store->updatePimItem( pimItems[ i ], HandlerHelper::collectionFromIdOrName( value ) ) )
          return failureResponse( "Unable to move item." );
      } else if ( command == "REMOTEID" ) {
        if ( !store->updatePimItem( pimItems[i], QString::fromUtf8( value ) ) )
          return failureResponse( "Unable to change remote id for item." );
      } else if ( command == "DIRTY" ) {
        PimItem item = pimItems.at( i );
        item.setDirty( false );
        if ( !item.update() )
          return failureResponse( "Unable to update item dirtyness" );
      } else {
        Part part;
        int version = 0;
        QByteArray plainCommand;

        ImapParser::splitVersionedKey( command, plainCommand, version );

        SelectQueryBuilder<Part> qb;
        qb.addValueCondition( Part::pimItemIdColumn(), Query::Equals, pimItems[ i ].id() );
        qb.addValueCondition( Part::nameColumn(), Query::Equals, QString::fromUtf8( plainCommand ) );
        if ( !qb.exec() )
          return failureResponse( "Unable to check item part existence" );
        Part::List result = qb.result();

        if ( !result.isEmpty() ) {
          part = result.first();
        }

        // only update if part contents are not yet in the storage
        QByteArray origData = PartHelper::translateData( part );
        if ( origData != value )
        {
          part.setName( QString::fromUtf8( plainCommand ) );
          part.setVersion( version );
          part.setPimItemId( pimItems[ i ].id() );
          if ( part.isValid() ) {
            if ( !PartHelper::update( &part, value, value.size() ) )
              return failureResponse( "Unable to update item part" );
          } else {
            qDebug() << "insert from Store::handleLine";
            part.setData( value );
            part.setDatasize( value.size() );
            if ( !PartHelper::insert(&part) )
              return failureResponse( "Unable to add item part" );
          }
          store->updatePimItem( pimItems[ i ] );
        }
      }

      if ( !silent ) {
        QList<Flag> flags = pimItems[ i ].flags();
        QStringList flagList;
        for ( int j = 0; j < flags.count(); ++j )
          flagList.append( flags[ j ].name() );

        Response response;
        response.setUntagged();
        // IMAP protocol violation: should actually be the sequence number
        response.setString( QByteArray::number( pimItems[i].id() ) + " FETCH (FLAGS (" + flagList.join( QLatin1String(" ") ).toUtf8() + "))" );
        emit responseAvailable( response );
      }
    }
  }

  if ( !transaction.commit() )
    return failureResponse( "Cannot commit transaction." );

  QString datetime = QLocale::c().toString( modificationtime, QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );

  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( "DATETIME " + ImapParser::quote( datetime.toUtf8() ) + " STORE completed" );

  emit responseAvailable( response );
  deleteLater();

  return true;
}

void Store::parseCommandStream()
{
  QByteArray buffer = m_streamParser->readString();
  if ( buffer != "STORE" ) {
    //put back what was read
    m_streamParser->insertData(' ' + buffer + ' ');
  }

  mItemSet = m_streamParser->readSequenceSet();
  if ( mItemSet.isEmpty() )
    throw HandlerException( "No item specified" );

  // parse revision
  buffer = m_streamParser->readString(); // skip 'REV'
  if ( buffer == "REV" ) {
    bool ok;
    mPreviousRevision = m_streamParser->readNumber( &ok );
    if ( !ok ) {
      throw HandlerException( "Unable to parse item revision number." );
    }
  }

  // parse size
  buffer = m_streamParser->readString();
  if ( buffer == "SIZE" ) {
    bool ok;
    mSize = m_streamParser->readNumber( &ok );
    if ( !ok ) {
      throw HandlerException( "Unable to parse the size.");
    }
  }
}

#include "store.moc"
