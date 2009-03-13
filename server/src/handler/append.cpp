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
#include "../../libs/imapparser_p.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"

#include <QtCore/QDebug>
#include <QLocale>

using namespace Akonadi;

Append::Append()
    : Handler()
{
}


Append::~Append()
{
}

bool Akonadi::Append::handleLine(const QByteArray& line )
{
    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        message literal
    //
    // Syntax:
    // append = "APPEND" SP mailbox SP size [SP flag-list] [SP date-time] SP literal

    const int startOfCommand = line.indexOf( ' ' ) + 1;
    const int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;

    const int startOfSize = ImapParser::parseString( line, m_mailbox, startOfMailbox ) + 1;

    QString size;
    const int startOfFlags = ImapParser::parseString( line, size, startOfSize ) + 1;
    m_size = size.toLongLong();

    QString data;
    ImapParser::parseString( line, data, startOfSize);
    m_size = data.toLongLong();

    // parse optional flag parenthesized list
    // Syntax:
    // flag-list      = "(" [flag *(SP flag)] ")"
    // flag           = "\Answered" / "\Flagged" / "\Deleted" / "\Seen" /
    //                  "\Draft" / flag-keyword / flag-extension
    //                    ; Does not include "\Recent"
    // flag-extension = "\" atom
    // flag-keyword   = atom
    int startOfDateTime = startOfFlags;
    if ( line[startOfFlags] == '(' ) {
        startOfDateTime = ImapParser::parseParenthesizedList( line, m_flags, startOfFlags ) + 1;
    }

    // parse optional date/time string
    int startOfLiteral = startOfDateTime;
    if ( line[startOfDateTime] == '"' ) {
        startOfLiteral = ImapParser::parseDateTime( line, m_dateTime, startOfDateTime );
        m_dateTime = m_dateTime.toUTC();
        // FIXME Should we return an error if m_dateTime is invalid?
    } else {
        // if date/time is not given then it will be set to the current date/time
        // converted to UTC.
        m_dateTime = QDateTime::currentDateTime().toUTC();
    }

    ImapParser::parseString( line, m_data, startOfLiteral );
    return commit();
}

bool Akonadi::Append::commit()
{
    Response response;

    DataStore *db = connection()->storageBackend();
    Transaction transaction( db );

    Collection col = HandlerHelper::collectionFromIdOrName( m_mailbox );
    if ( !col.isValid() )
      return failureResponse( "Unknown collection." );

    QByteArray mt;
    QByteArray remote_id;
    QList<QByteArray> flags;
    foreach( const QByteArray &flag, m_flags ) {
      if ( flag.startsWith( "\\MimeType" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.indexOf( ']', pos1 );
        mt = flag.mid( pos1 + 1, pos2 - pos1 - 1 );
      } else if ( flag.startsWith( "\\RemoteId" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.indexOf( ']', pos1 );
        remote_id = flag.mid( pos1 + 1, pos2 - pos1 - 1 );
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
//     qDebug() << "Data appended " << m_data;
    part.setPimItemId( item.id() );
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

bool Append::supportsStreamParser()
{
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

  qDebug() << "Append::parseStream";
  QByteArray tmp = m_streamParser->readString(); // skip command
  if (tmp != "APPEND") {
    //put back what was read
    m_streamParser->insertData(' ' + tmp + ' ');
  }

  m_mailbox = m_streamParser->readString();

  bool ok = false;
  m_size = m_streamParser->readNumber( &ok );

  //FIXME: Andras: why do we read another "size" here ???
/*
  QString data;
  ImapParser::parseString( line, data, startOfSize);
  m_size = data.toLongLong();
*/
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

  m_data = m_streamParser->readString();
  return commit();
}

#include "append.moc"
