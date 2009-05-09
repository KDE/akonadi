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
#include <QFile>

using namespace Akonadi;

Store::Store( Scope::SelectionScope scope )
  : Handler()
  , mPos( 0 )
  , mPreviousRevision( -1 )
  , mSize( 0 )
  , mUidStore( scope == Scope::Uid )
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
  parseCommandStream();
  DataStore *store = connection()->storageBackend();
  Transaction transaction( store, false );
   // Set the same modification time for each item.
  QDateTime modificationtime = QDateTime::currentDateTime().toUTC();

  if (m_streamParser->hasList())
  {

    QList<QByteArray> result;
    m_streamParser->stripLeadingSpaces();
    QList<PimItem> pimItems;

    int count = 0;
    QByteArray command;
    bool silent = false;
    bool firstCommand = true;

    Q_FOREVER {
      QByteRef nextChar = m_streamParser->readChar();
      if ( nextChar == '(' ) {
        ++count;
        if ( count > 1 )  {
          //handle commands that have a list of arguments
          m_streamParser->insertData( "(" );
          QList<QByteArray> value = m_streamParser->readParenthesizedList();

          Operation op = Replace;

          if ( command.startsWith( '+' ) ) {
            op = Add;
            command = command.mid( 1 );
          } else if ( command.startsWith( '-' ) ) {
            op = Delete;
            command = command.mid( 1 );
          }

          for ( int i = 0; i < pimItems.count(); ++i ) {
            if ( command == "FLAGS" ) {
              if ( op == Replace ) {
                if ( !replaceFlags( pimItems[ i ], value ) )
                  return failureResponse( "Unable to replace item flags." );
              } else if ( op == Add ) {
                if ( !addFlags( pimItems[ i ], value ) )
                  return failureResponse( "Unable to add item flags." );
              } else if ( op == Delete ) {
                if ( !deleteFlags( pimItems[ i ], value ) )
                  return failureResponse( "Unable to remove item flags." );
              }
            } else if ( command == "PARTS" ) {
              if ( op == Delete ) {
                if ( !store->removeItemParts( pimItems[ i ], value ) )
                  return failureResponse( "Unable to remove item parts." );
              }
            }
            if ( !silent ) {
              sendPimItemResponse( pimItems[i] );
            }
          }
          command = QByteArray();
          silent = false;
          count--;
        }
        continue;
      }

      if ( nextChar == ')' ) {
        if ( count > 1 ) {
          throw HandlerException( "Syntax error" );
        }
        break; //end of list reached
      }

      if ( nextChar == ' ' ) {
        continue;
      }


      if ( count == 1 ) {
        //put back the previously read character, as we will read a string or literal now
        QByteArray ba;
        ba.append( nextChar );
        m_streamParser->insertData( ba );

        command = m_streamParser->readString();

        if ( command.endsWith( ".SILENT" ) ) {
          command.chop( 7 );
          silent = true;
        }

        // check if this is a item move, and fetch all items in that case
        //TODO: strictly only needed for inter-resource moves
        //NOTE: COLLECTION must be ALWAYS the first in the list
        if ( command == "COLLECTION" ) {
          ItemRetriever retriever( connection() );
          retriever.setItemSet( mItemSet, mUidStore );
          retriever.setRetrieveFullPayload( true );
          retriever.exec();
        }

        if (!command.isEmpty() && firstCommand) {
          //this is executed only once, before the first command is processed
          firstCommand = false;

          transaction.begin();
          SelectQueryBuilder<PimItem> qb;
          ItemQueryHelper::itemSetToQuery( mItemSet, mUidStore, connection(), qb );
          if ( !qb.exec() )
            return failureResponse( "Unable to retrieve items" );
          pimItems = qb.result();
          if ( pimItems.isEmpty() )
            return failureResponse( "No items found" );


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
        }

        //go through all the pim items and handle the commands
        for ( int i = 0; i < pimItems.count(); ++i ) {
          bool storeData = false;
          QByteArray value;
          if ( command == "COLLECTION" ) {
            value = m_streamParser->readString();
            if ( !store->updatePimItem( pimItems[ i ], HandlerHelper::collectionFromIdOrName( value ) ) )
              return failureResponse( "Unable to move item." );
          } else
          if ( command == "REMOTEID" ) {
            value = m_streamParser->readString();
            if ( pimItems[i].remoteId() != QString::fromUtf8( value ) ) {
              if ( !connection()->isOwnerResource( pimItems[i] ) )
                throw HandlerException( "Only resources can modify remote identifiers" );
              if ( !store->updatePimItem( pimItems[i], QString::fromUtf8( value ) ) )
                return failureResponse( "Unable to change remote id for item." );
            }
          } else
          if ( command == "DIRTY" ) {
            value = m_streamParser->readString();
            PimItem item = pimItems.at( i );
            item.setDirty( false );
            if ( !item.update() )
              return failureResponse( "Unable to update item dirtyness" );
          } else
          if (command == "PLD:RFC822") {
            storeData = true;
            if ( m_streamParser->hasLiteral() ) {
              qint64 dataSize = m_streamParser->remainingLiteralSize();
              bool storeInFile = (DbConfig::useExternalPayloadFile() && dataSize > DbConfig::sizeThreshold() );
              //actual case when streaming storage is used: external payload is enabled, data is big enough in a literal
              if (storeInFile) {
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

                value = m_streamParser->readLiteralPart();
                part.setExternal( true ); //the part WILL be external

                part.setName( QString::fromUtf8( plainCommand ) );
                part.setVersion( version );
                part.setPimItemId( pimItems[ i ].id() );
                if ( part.isValid() ) {
                  if ( !PartHelper::update( &part, value, dataSize ) )
                    return failureResponse( "Unable to update item part" );
                } else {
                  qDebug() << "insert from Store::handleLine";
                  part.setData( value );
                  part.setDatasize( value.size() );
                  if ( !PartHelper::insert(&part) )
                    return failureResponse( "Unable to add item part" );
                }

                //the actual streaming code, reads from the parser, writes immediately to the file
                QString fileName = QString::fromUtf8( part.data() );
                QFile file(fileName);

                if (file.open( QIODevice::WriteOnly | QIODevice::Append )) {
                  while (!m_streamParser->atLiteralEnd()) {
                    value = m_streamParser->readLiteralPart();
                    file.write( value );
                  }
                  file.close();
                } else
                {
                  return failureResponse( "Unable to update item part" );
                }

                store->updatePimItem( pimItems[ i ] );

                storeData = false; //processed
              } else {
                //don't write in streaming way as the data goes to the database
                while (!m_streamParser->atLiteralEnd()) {
                  value += m_streamParser->readLiteralPart();
                }
              }
            } else { //not a literal
              value = m_streamParser->readString();
            }
          } else { //every other command
            storeData = true;
            value = m_streamParser->readString();
          }
          if (!command.isEmpty() && storeData) //a command whose data needs to be stored, except when the data is big and is read in parts (see above)
          {
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
            if ( origData != value ) {
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
            sendPimItemResponse( pimItems[i] );
          }
        }

      }
    }

  if ( !transaction.commit() )
    return failureResponse( "Cannot commit transaction." );

  }

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
  mItemSet = m_streamParser->readSequenceSet();
  if ( mItemSet.isEmpty() )
    throw HandlerException( "No item specified" );

  // parse revision
  QByteArray buffer = m_streamParser->readString(); // skip 'REV'
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

void Store::sendPimItemResponse( const PimItem &pimItem )
{
  QList<Flag> flags = pimItem.flags();
  QStringList flagList;
  for ( int j = 0; j < flags.count(); ++j )
    flagList.append( flags[ j ].name() );

  Response response;
  response.setUntagged();
        // IMAP protocol violation: should actually be the sequence number
  response.setString( QByteArray::number( pimItem.id() ) + " FETCH (FLAGS (" + flagList.join( QLatin1String(" ") ).toUtf8() + "))" );
  emit responseAvailable( response );
}

#include "store.moc"
