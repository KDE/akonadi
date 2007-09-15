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
#include "imapparser.h"
#include "handlerhelper.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "protocol.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"

#include <QtCore/QDebug>

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
    // append = "APPEND" SP mailbox [SP flag-list] [SP date-time] SP literal

    const int startOfCommand = line.indexOf( ' ' ) + 1;
    const int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    const int startOfFlags = ImapParser::parseString( line, m_mailbox, startOfMailbox ) + 1;

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
        startOfDateTime = line.indexOf( ')', startOfFlags + 1 ) + 2;
        m_flags = line.mid( startOfFlags + 1,
                            startOfDateTime - ( startOfFlags + 1 ) - 2 ).split(' ');
    }
    m_flags.append( "\\Recent" ); // the Recent flag always has to be set

    // parse optional date/time string
    int startOfLiteral = startOfDateTime;
    if ( line[startOfDateTime] == '"' ) {
        startOfLiteral = ImapParser::parseDateTime( line, m_dateTime, startOfDateTime );
        // FIXME Should we return an error if m_dateTime is invalid?
    }
    // if date/time is not given then it will be set to the current date/time
    // by the database

    ImapParser::parseString( line, m_data, startOfLiteral );
    return commit();
}

bool Akonadi::Append::commit()
{
    Response response;

    DataStore *db = connection()->storageBackend();
    Transaction transaction( db );

    Location l = HandlerHelper::collectionFromIdOrName( m_mailbox );
    if ( !l.isValid() )
      return failureResponse( "Unknown collection." );

    QByteArray mt;
    QByteArray remote_id;
    QList<QByteArray> flags;
    foreach( QByteArray flag, m_flags ) {
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
      return failureResponse( QString::fromLatin1( "Unknown mime type '%1'.").arg( QString::fromLatin1( mt ) ) );
    }
    PimItem item;

    // wrap data into a part
    Part part;
    part.setName( QLatin1String( AKONADI_PART_BODY ) );
    part.setData( m_data );
    part.setPimItemId( item.id() );
    QList<Part> parts;
    parts.append( part );

    bool ok = db->appendPimItem( parts, mimeType, l, m_dateTime, remote_id, item );
    response.setTag( tag() );
    if ( !ok ) {
        return failureResponse( "Append failed" );
    }

    // set message flags
    if ( !db->appendItemFlags( item, flags ) )
      return failureResponse( "Unable to append item flags." );

    // the message was appended; now we have to update the counts
    const int existsChange = +1;
    const int recentChange = +1;
    int unseenChange = 0;
    if ( !flags.contains( "\\Seen" ) )
        unseenChange = +1;
    // int firstUnseen = ?; // can't be updated atomically, so we probably have to
                            // recalculate it each time it's needed
    if ( !db->updateLocationCounts( l, existsChange, recentChange, unseenChange ) )
      return failureResponse( "Unable to update collection counts." );

    // TODO if the mailbox is currently selected, the normal new message
    //      actions SHOULD occur.  Specifically, the server SHOULD notify the
    //      client immediately via an untagged EXISTS response.

    if ( !transaction.commit() )
        return failureResponse( "Unable to commit transaction." );

    response.setTag( tag() );
    response.setUserDefined();
    response.setString( "[UIDNEXT " + QByteArray::number( item.id() ) + ']' );
    emit responseAvailable( response );

    response.setSuccess();
    response.setString( "Append completed" );
    emit responseAvailable( response );
    deleteLater();

    return true;
}
