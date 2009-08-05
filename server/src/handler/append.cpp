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
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "storage/parthelper.h"
#include "storage/dbconfig.h"

#include <QtCore/QDebug>
#include <QLocale>
#include <QFile>
#include <QTemporaryFile>

using namespace Akonadi;

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
      storeInFile = DbConfig::useExternalPayloadFile() && dataSize > DbConfig::sizeThreshold();
      if ( storeInFile ) {
        if ( !tmpFile.open() ) {
          storeInFile =  false;
        }
      }
      while ( !m_streamParser->atLiteralEnd() ) {
        if ( !storeInFile ) {
          m_data += m_streamParser->readLiteralPart();
        } else
        {
          m_data = m_streamParser->readLiteralPart();
          tmpFile.write( m_data );
        }
      }
      if ( storeInFile ) {
        tmpFile.close();
        m_data = "";
      }
    } else {
      m_data = m_streamParser->readString();
    }

    Response response;

    DataStore *db = connection()->storageBackend();
    Transaction transaction( db );

    Collection col = HandlerHelper::collectionFromIdOrName( m_mailbox );
    if ( !col.isValid() )
      return failureResponse( "Unknown collection." );

    QByteArray mt;
    QString remote_id;
    QList<QByteArray> flags;
    foreach( const QByteArray &flag, m_flags ) {
      if ( flag.startsWith( "\\MimeType" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.indexOf( ']', pos1 );
        mt = flag.mid( pos1 + 1, pos2 - pos1 - 1 );
      } else if ( flag.startsWith( "\\RemoteId" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.lastIndexOf( ']' );
        remote_id = QString::fromUtf8( flag.mid( pos1 + 1, pos2 - pos1 - 1 ) );
      } else
        flags << flag;
    }
    // standard imap does not know this attribute, so that's mail
    if ( mt.isEmpty() ) mt = "message/rfc822";
    MimeType mimeType = MimeType::retrieveByName( QString::fromLatin1( mt ) );
    if ( !mimeType.isValid() ) {
      MimeType m( QString::fromLatin1( mt ) );
      if ( !m.insert() )
        return failureResponse( QString::fromLatin1( "Unable to create mimetype '%1'.").arg( QString::fromLatin1( mt ) ) );
      mimeType = m;
    }

    PimItem item;
    item.setRev( 0 );
    item.setSize( m_size );
    item.setDatetime( m_dateTime );

    // wrap data into a part
    Part part;
    part.setName( QLatin1String( "PLD:RFC822" ) );
    part.setData( m_data );
    part.setPimItemId( item.id() );
    if (storeInFile) {
      part.setDatasize( dataSize );
      part.setExternal( true );//force external storage
    }

    QList<Part> parts;
    parts.append( part );

    bool ok = db->appendPimItem( parts, mimeType, col, m_dateTime, remote_id, item );
    response.setTag( tag() );
    if ( !ok ) {
        return failureResponse( "Append failed" );
    }

    // set message flags
    if ( !db->appendItemFlags( item, flags, false, col ) )
      return failureResponse( "Unable to append item flags." );

    if (storeInFile) {
      part.setExternal( true );
      //the new item was just created and the transaction is not yet committed, so delete + overwrite should be safe, as no
      //client knows about the item yet
      QString fileName = QString::fromUtf8( parts[0].data() );
      QFile f( fileName );
      if ( !f.remove() ) {
        return failureResponse( "Unable to remove item part file" );
      }
      if ( !tmpFile.copy( fileName ) ) {
        return failureResponse( "Unable to copy item part data from the temporary file" );
      }
    }

    // TODO if the mailbox is currently selected, the normal new message
    //      actions SHOULD occur.  Specifically, the server SHOULD notify the
    //      client immediately via an untagged EXISTS response.

    if ( !transaction.commit() )
        return failureResponse( "Unable to commit transaction." );

    // Date time is always stored in UTC time zone by the server.
    QString datetime = QLocale::c().toString( item.datetime(), QLatin1String( "dd-MMM-yyyy hh:mm:ss +0000" ) );

    QByteArray res( "[UIDNEXT " + QByteArray::number( item.id() ) + " " );
    res.append( "DATETIME " + ImapParser::quote( datetime.toUtf8() ) );
    res.append( ']' );

    response.setTag( tag() );
    response.setUserDefined();
    response.setString( res );
    emit responseAvailable( response );

    response.setSuccess();
    response.setString( "Append completed" );
    emit responseAvailable( response );
    deleteLater();

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
  // flag           = "\Answered" / "\Flagged" / "\Deleted" / "\Seen" /
    //                  "\Draft" / flag-keyword / flag-extension
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

#include "append.moc"
