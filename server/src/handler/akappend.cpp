/***************************************************************************
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
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

#include "akappend.h"

#include "libs/imapparser_p.h"
#include "imapstreamparser.h"

#include "append.h"
#include "response.h"
#include "handlerhelper.h"

#include "akonadi.h"
#include "connection.h"
#include "preprocessormanager.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "storage/parttypehelper.h"
#include "storage/dbconfig.h"
#include "storage/parthelper.h"
#include "libs/protocol_p.h"

#include <QtCore/QDebug>

using namespace Akonadi;
using namespace Akonadi::Server;

AkAppend::AkAppend()
    : Handler()
{
}

AkAppend::~AkAppend()
{
}

bool AkAppend::buildPimItem( PimItem &item, const QByteArray &mailbox, qint64 size, const QList<QByteArray> &flags, const QDateTime &dateTime, QList<QByteArray> &itemFlags )
{
    Response response;

    const Collection col = HandlerHelper::collectionFromIdOrName( mailbox );
    if ( !col.isValid() ) {
      throw HandlerException( QByteArray( "Unknown collection for '" ) + mailbox + QByteArray( "'." ) );
    }

    QByteArray mt;
    QString remote_id;
    QString remote_revision;
    QString gid;
    Q_FOREACH ( const QByteArray &flag, flags ) {
      if ( flag.startsWith( AKONADI_FLAG_MIMETYPE ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.indexOf( ']', pos1 );
        mt = flag.mid( pos1 + 1, pos2 - pos1 - 1 );
      } else if ( flag.startsWith( AKONADI_FLAG_REMOTEID ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.lastIndexOf( ']' );
        remote_id = QString::fromUtf8( flag.mid( pos1 + 1, pos2 - pos1 - 1 ) );
      } else if ( flag.startsWith( AKONADI_FLAG_REMOTEREVISION ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.lastIndexOf( ']' );
        remote_revision = QString::fromUtf8( flag.mid( pos1 + 1, pos2 - pos1 - 1 ) );
      } else if ( flag.startsWith( AKONADI_FLAG_GID ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.lastIndexOf( ']' );
        gid = QString::fromUtf8( flag.mid( pos1 + 1, pos2 - pos1 - 1 ) );
      } else {
        itemFlags << flag;
      }
    }
    // standard imap does not know this attribute, so that's mail
    if ( mt.isEmpty() ) {
      mt = "message/rfc822";
    }
    MimeType mimeType = MimeType::retrieveByName( QString::fromLatin1( mt ) );
    if ( !mimeType.isValid() ) {
      MimeType m( QString::fromLatin1( mt ) );
      if ( !m.insert() ) {
        return failureResponse( QByteArray( "Unable to create mimetype '" ) + mt + QByteArray( "'." ) );
      }
      mimeType = m;
    }

    item.setRev( 0 );
    item.setSize( size );
    item.setMimeTypeId( mimeType.id() );
    item.setCollectionId( col.id() );
    if ( dateTime.isValid() ) {
      item.setDatetime( dateTime );
    }
    if ( remote_id.isEmpty() ) {
      // from application
      item.setDirty( true );
    } else {
      // from resource
      item.setRemoteId( remote_id );
      item.setDirty( false );
    }
    item.setRemoteRevision( remote_revision );
    item.setGid( gid );
    item.setAtime( QDateTime::currentDateTime() );

    return true;
}

// This is used for clients that don't support item streaming
bool AkAppend::readParts( PimItem &pimItem )
{

  // parse part specification
  QVector<QPair<QByteArray, QPair<qint64, int> > > partSpecs;
  QByteArray partName = "";
  qint64 partSize = -1;
  qint64 partSizes = 0;
  bool ok = false;

  qint64 realSize = pimItem.size();

  const QList<QByteArray> list = m_streamParser->readParenthesizedList();
  Q_FOREACH ( const QByteArray &item, list ) {
    if ( partName.isEmpty() && partSize == -1 ) {
      partName = item;
      continue;
    }
    if ( item.startsWith( ':' ) ) {
      int pos = 1;
      ImapParser::parseNumber( item, partSize, &ok, pos );
      if ( !ok ) {
        partSize = 0;
      }

      int version = 0;
      QByteArray plainPartName;
      ImapParser::splitVersionedKey( partName, plainPartName, version );

      partSpecs.append( qMakePair( plainPartName, qMakePair( partSize, version ) ) );
      partName = "";
      partSizes += partSize;
      partSize = -1;
    }
  }

  realSize = qMax( partSizes, realSize );

  const QByteArray allParts = m_streamParser->readString();

  // chop up literal data in parts
  int pos = 0; // traverse through part data now
  QPair<QByteArray, QPair<qint64, int> > partSpec;
  Q_FOREACH ( partSpec, partSpecs ) {
    // wrap data into a part
    Part part;
    part.setPimItemId( pimItem.id() );
    part.setPartType( PartTypeHelper::fromFqName( partSpec.first ) );
    part.setData( allParts.mid( pos, partSpec.second.first ) );
    if ( partSpec.second.second != 0 ) {
      part.setVersion( partSpec.second.second );
    }
    part.setDatasize( partSpec.second.first );

    if ( !PartHelper::insert( &part ) ) {
      return failureResponse( "Unable to append item part" );
    }

    pos += partSpec.second.first;
  }

  if ( realSize != pimItem.size() ) {
    pimItem.setSize( realSize );
    pimItem.update();
  }

  return true;
}

bool AkAppend::parseStream()
{
    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        (partname literal)+
    //
    // Syntax:
    // x-akappend = "X-AKAPPEND" SP mailbox SP size [SP flag-list] [SP date-time] SP (partname SP literal)+

  const QByteArray mailbox = m_streamParser->readString();

  const qint64 size = m_streamParser->readNumber();

  // parse optional flag parenthesized list
  // Syntax:
  // flag-list      = "(" [flag *(SP flag)] ")"
  // flag           = "\ANSWERED" / "\FLAGGED" / "\DELETED" / "\SEEN" /
  //                  "\DRAFT" / flag-keyword / flag-extension
  //                    ; Does not include "\Recent"
  // flag-extension = "\" atom
  // flag-keyword   = atom
  QList<QByteArray> flags;
  if ( m_streamParser->hasList() ) {
    flags = m_streamParser->readParenthesizedList();
  }

  // parse optional date/time string
  QDateTime dateTime;
  if ( m_streamParser->hasDateTime() ) {
    dateTime = m_streamParser->readDateTime().toUTC();
    // FIXME Should we return an error if m_dateTime is invalid?
  } else {
    // if date/time is not given then it will be set to the current date/time
    // converted to UTC.
    dateTime = QDateTime::currentDateTime().toUTC();
  }

  // FIXME: The streaming/reading of all item parts can hold the transaction for
  // unnecessary long time -> should we wrap the PimItem into one transaction
  // and try to insert Parts independently? In case we fail to insert a part,
  // it's not a problem as it can be re-fetched at any time, except for attributes.
  DataStore *db = DataStore::self();
  Transaction transaction( db );

  QList<QByteArray> itemFlags;
  PimItem item;
  if ( !buildPimItem( item, mailbox, size, flags, dateTime, itemFlags ) ) {
    return false;
  }
  if ( !item.insert() ) {
    return failureResponse( "Failed to append item" );
  }

  // set message flags
  // This will hit an entry in cache inserted there in buildPimItem()
  const Collection col = HandlerHelper::collectionFromIdOrName( mailbox );
  const Flag::List flagList = HandlerHelper::resolveFlags( itemFlags );
  bool flagsChanged = false;
  if ( !db->appendItemsFlags( PimItem::List() << item, flagList, flagsChanged, false, col, true ) ) {
    return failureResponse( "Unable to append item flags." );
  }

  // Handle individual parts
  qint64 partSizes = 0;
  if ( connection()->capabilities().akAppendStreaming() ) {
    QByteArray error, partName /* unused */;
    qint64 partSize;
    m_streamParser->beginList();
    while ( !m_streamParser->atListEnd() ) {
      QByteArray command = m_streamParser->readString();
      if ( command.isEmpty() ) {
        throw HandlerException( "Syntax error" );
      }
      if ( !PartHelper::storeStreamedParts( command, m_streamParser, item, false, partName, partSize, error ) ) {
        return failureResponse( error );
      }
      partSizes += partSize;
    }

    // TODO: Try to avoid this addition query
    if ( partSizes > item.size() ) {
      item.setSize( partSizes );
      item.update();
    }

  } else {
    if ( !readParts( item ) ) {
      return false;
    }
  }

  // Preprocessing
  const bool doPreprocessing = PreprocessorManager::instance()->isActive();
  if ( doPreprocessing ) {
    Part hiddenAttribute;
    hiddenAttribute.setPimItemId( item.id() );
    hiddenAttribute.setPartType( PartTypeHelper::fromFqName( QString::fromLatin1( AKONADI_ATTRIBUTE_HIDDEN ) ) );
    hiddenAttribute.setData( QByteArray() );
    // TODO: Handle errors? Technically, this is not a critical issue as no data are lost
    PartHelper::insert( &hiddenAttribute );
  }

  // All SQL is done, let's commit!
  if ( !transaction.commit() ) {
    return failureResponse( "Failed to commit transaction" );
  }

  DataStore::self()->notificationCollector()->itemAdded( item, col );

  if ( doPreprocessing ) {
    // enqueue the item for preprocessing
    PreprocessorManager::instance()->beginHandleItem( item, db );
  }

  // Date time is always stored in UTC time zone by the server.
  const QString datetime = QLocale::c().toString( item.datetime(), QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );

  // ...aaaaaand done.
  Response response;
  response.setTag( tag() );
  response.setUserDefined();
  response.setString( "[UIDNEXT " + QByteArray::number( item.id() ) + " DATETIME " + ImapParser::quote( datetime.toUtf8() ) + ']' );
  Q_EMIT responseAvailable( response );

  response.setSuccess();
  response.setString( "Append completed" );
  Q_EMIT responseAvailable( response );
  return true;
}
