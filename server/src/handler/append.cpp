/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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

#include "append.h"
#include "response.h"
#include "handlerhelper.h"
#include "libs/imapparser_p.h"
#include "imapstreamparser.h"

#include "akonadi.h"
#include "connection.h"
#include "preprocessormanager.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "storage/parthelper.h"
#include "storage/dbconfig.h"
#include "storage/parttypehelper.h"

#include <QtCore/QDebug>
#include <QLocale>
#include <QFile>
#include <QTemporaryFile>

using namespace Akonadi;
using namespace Akonadi::Server;

Append::Append()
    : Handler()
{
}

Append::~Append()
{
}

bool Append::commit()
{
    bool storeInFile = false;
    qint64 dataSize = 0;
    QTemporaryFile tmpFile;

    //need to read out the data before the below code to avoid hangs, probably at some place the eventloop is reentered
    //and more data is read into the stream and that causes the problems.
    if ( m_streamParser->hasLiteral() ) {
      dataSize = m_streamParser->remainingLiteralSize();
      m_size = qMax( m_size, dataSize );
      storeInFile = dataSize > DbConfig::configuredDatabase()->sizeThreshold();
      if ( storeInFile ) {
        try {
          PartHelper::streamToFile( m_streamParser, tmpFile );
        } catch ( const PartHelperException &e ) {
          return failureResponse( e.what() );
        }
      } else {
        while ( !m_streamParser->atLiteralEnd() ) {
          m_data += m_streamParser->readLiteralPart();
        }
      }
    } else {
      m_data = m_streamParser->readString();
    }

    Response response;

    DataStore *db = connection()->storageBackend();
    Transaction transaction( db );

    Collection col = HandlerHelper::collectionFromIdOrName( m_mailbox );
    if ( !col.isValid() ) {
      return failureResponse( QByteArray( "Unknown collection for '" ) + m_mailbox + QByteArray( "'." ) );
    }

    QByteArray mt;
    QString remote_id;
    QString remote_revision;
    QString gid;
    QVector<QByteArray> flags;
    Q_FOREACH ( const QByteArray &flag, m_flags ) {
      if ( flag.startsWith( "\\MimeType" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.indexOf( ']', pos1 );
        mt = flag.mid( pos1 + 1, pos2 - pos1 - 1 );
      } else if ( flag.startsWith( "\\RemoteId" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.lastIndexOf( ']' );
        remote_id = QString::fromUtf8( flag.mid( pos1 + 1, pos2 - pos1 - 1 ) );
      } else if ( flag.startsWith( "\\RemoteRevision" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.lastIndexOf( ']' );
        remote_revision = QString::fromUtf8( flag.mid( pos1 + 1, pos2 - pos1 - 1 ) );
      } else if ( flag.startsWith( "\\Gid" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.lastIndexOf( ']' );
        gid = QString::fromUtf8( flag.mid( pos1 + 1, pos2 - pos1 - 1 ) );
      } else {
        flags << flag;
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
        return failureResponse( QByteArray( "Unable to create mimetype '" ) +  mt + QByteArray( "'." ) );
      }
      mimeType = m;
    }

    PimItem item;
    item.setRev( 0 );
    item.setSize( m_size );
    item.setDatetime( m_dateTime );

    // wrap data into a part
    Part part;
    part.setPartType( PartTypeHelper::fromFqName( QLatin1String("PLD"), QLatin1String("RFC822") ) );
    part.setData( m_data );
    part.setPimItemId( item.id() );
    part.setDatasize( dataSize );

    QVector<Part> parts;
    parts.append( part );

    // If we have active preprocessors then we also set the hidden attribute
    // for the UI and we enqueue the item for preprocessing.
    bool doPreprocessing = PreprocessorManager::instance()->isActive();
    //akDebug() << "Append handler: doPreprocessing is" << doPreprocessing;
    if ( doPreprocessing ) {
      Part hiddenAttribute;
      hiddenAttribute.setPartType( PartTypeHelper::fromFqName( QLatin1String("ATR"), QLatin1String("HIDDEN") ) );
      hiddenAttribute.setData( QByteArray() );
      hiddenAttribute.setPimItemId( item.id() );
      hiddenAttribute.setDatasize( 0 );
      parts.append( hiddenAttribute );
    }

    bool ok = db->appendPimItem( parts, mimeType, col, m_dateTime, remote_id, remote_revision, gid, item );
    response.setTag( tag() );
    if ( !ok ) {
        return failureResponse( "Append failed" );
    }

    // set message flags
    const Flag::List flagList = HandlerHelper::resolveFlags( flags );
    bool flagsChanged = false;
    if ( !db->appendItemsFlags( PimItem::List() << item, flagList, &flagsChanged, false, col ) ) {
      return failureResponse( "Unable to append item flags." );
    }

    if ( storeInFile ) {
      const QString fileName = PartHelper::resolveAbsolutePath( parts[0].data() );

      //the new item was just created and the transaction is not yet committed, so delete + overwrite should be safe, as no
      //client knows about the item yet
      PartHelper::removeFile( fileName );

      if ( !tmpFile.copy( fileName ) ) {
        return failureResponse( "Unable to copy item part data from the temporary file" );
      }
    }

    // TODO if the mailbox is currently selected, the normal new message
    //      actions SHOULD occur.  Specifically, the server SHOULD notify the
    //      client immediately via an untagged EXISTS response.

    if ( !transaction.commit() ) {
        return failureResponse( "Unable to commit transaction." );
    }

    if ( doPreprocessing ) {
      // enqueue the item for preprocessing
      PreprocessorManager::instance()->beginHandleItem( item, db );
    }

    // Date time is always stored in UTC time zone by the server.
    QString datetime = QLocale::c().toString( item.datetime(), QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );

    QByteArray res( "[UIDNEXT " + QByteArray::number( item.id() ) + ' ' );
    res.append( "DATETIME " + ImapParser::quote( datetime.toUtf8() ) );
    res.append( ']' );

    response.setTag( tag() );
    response.setUserDefined();
    response.setString( res );
    Q_EMIT responseAvailable( response );

    response.setSuccess();
    response.setString( "Append completed" );
    Q_EMIT responseAvailable( response );
    return true;
}

bool Append::parseStream()
{
    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        message literal
  //
    // Syntax:
    // append = "APPEND" SP mailbox SP size [SP flag-list] [SP date-time] SP literal

  m_mailbox = m_streamParser->readString();

  m_size = m_streamParser->readNumber();

    // parse optional flag parenthesized list
    // Syntax:
    // flag-list      = "(" [flag *(SP flag)] ")"
    // flag           = "\ANSWERED" / "\FLAGGED" / "\DELETED" / "\SEEN" /
    //                  "\DRAFT" / flag-keyword / flag-extension
    //                    ; Does not include "\Recent"
    // flag-extension = "\" atom
    // flag-keyword   = atom
  if ( m_streamParser->hasList() ) {
    m_flags = m_streamParser->readParenthesizedList();
  }

    // parse optional date/time string
  if ( m_streamParser->hasDateTime() ) {
    m_dateTime = m_streamParser->readDateTime();
    m_dateTime = m_dateTime.toUTC();
        // FIXME Should we return an error if m_dateTime is invalid?
  } else {
        // if date/time is not given then it will be set to the current date/time
        // converted to UTC.
    m_dateTime = QDateTime::currentDateTime().toUTC();
  }

  return commit();
}
