/***************************************************************************
 *   Copyright (C) 2006 by Till Adam <adam@kde.org>                        *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/
#include <QDebug>

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"

#include "append.h"
#include "response.h"

using namespace Akonadi;

Append::Append()
    : Handler(), m_size(-1)
{
}


Append::~Append()
{
}


static QDateTime parseDateTime( const QByteArray & s )
{
    // Syntax:
    // date-time      = DQUOTE date-day-fixed "-" date-month "-" date-year
    //                  SP time SP zone DQUOTE
    // date-day-fixed = (SP DIGIT) / 2DIGIT
    //                    ; Fixed-format version of date-day
    // date-month     = "Jan" / "Feb" / "Mar" / "Apr" / "May" / "Jun" /
    //                  "Jul" / "Aug" / "Sep" / "Oct" / "Nov" / "Dec"
    // date-year      = 4DIGIT
    // time           = 2DIGIT ":" 2DIGIT ":" 2DIGIT
    //                    ; Hours minutes seconds
    // zone           = ("+" / "-") 4DIGIT
    //                    ; Signed four-digit value of hhmm representing
    //                    ; hours and minutes east of Greenwich (that is,
    //                    ; the amount that the given time differs from
    //                    ; Universal Time).  Subtracting the timezone
    //                    ; from the given time will give the UT form.
    //                    ; The Universal Time zone is "+0000".
    // Example : "28-May-2006 01:03:35 +0200"
    // Position: 0123456789012345678901234567
    //                     1         2
    QDateTime dateTime;
    bool ok = true;
    const int day = ( s[1] == ' ' ? s[2] - '0' // single digit day
                                  : s.mid( 1, 2 ).toInt( &ok ) );
    if ( !ok ) return dateTime;
    const QByteArray shortMonthNames( "janfebmaraprmayjunjulaugsepoctnovdec" );
    int month = shortMonthNames.indexOf( s.mid( 4, 3 ).toLower() );
    if ( month == -1 ) return dateTime;
    month = month / 3 + 1;
    const int year = s.mid( 8, 4 ).toInt( &ok );
    if ( !ok ) return dateTime;
    const int hours = s.mid( 13, 2 ).toInt( &ok );
    if ( !ok ) return dateTime;
    const int minutes = s.mid( 16, 2 ).toInt( &ok );
    if ( !ok ) return dateTime;
    const int seconds = s.mid( 19, 2 ).toInt( &ok );
    if ( !ok ) return dateTime;
    const int tzhh = s.mid( 23, 2 ).toInt( &ok );
    if ( !ok ) return dateTime;
    const int tzmm = s.mid( 25, 2 ).toInt( &ok );
    if ( !ok ) return dateTime;
    int tzsecs = tzhh*60*60 + tzmm*60;
    if ( s[22] == '-' ) tzsecs = -tzsecs;
    const QDate date( year, month, day );
    const QTime time( hours, minutes, seconds );
    dateTime = QDateTime( date, time, Qt::UTC );
    if ( !dateTime.isValid() ) return dateTime;
    dateTime.addSecs( -tzsecs );
    return dateTime;
}


bool Akonadi::Append::handleContinuation( const QByteArray& line )
{
    m_data += line;
    m_size -= line.size();
    if ( !allDataRead() )
        return false;
    return commit();
}

bool Akonadi::Append::handleLine(const QByteArray& line )
{
    if ( inContinuation() )
        return handleContinuation( line );

    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        message literal
    //
    // Syntax:
    // append = "APPEND" SP mailbox [SP flag-list] [SP date-time] SP literal

    const int startOfCommand = line.indexOf( ' ' ) + 1;
    const int startOfMailbox = line.indexOf( ' ', startOfCommand ) + 1;
    const int startOfFlags = line.indexOf( ' ', startOfMailbox ) + 1;
    m_mailbox = stripQuotes( line.mid( startOfMailbox, startOfFlags - startOfMailbox -1 ) );

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
        startOfLiteral = line.indexOf( '{', startOfDateTime + 1 );
        m_dateTime = parseDateTime( line.mid( startOfDateTime, startOfLiteral - startOfDateTime - 1 ) );
        // FIXME Should we return an error if m_dateTime is invalid?
    }
    // if date/time is not given then it will be set to the current date/time
    // by the database

    // finally parse the message literal
    const int startOfSize = startOfLiteral + 1;
    m_size = line.mid( startOfSize, line.indexOf('}') - startOfSize ).toInt();

    if ( !allDataRead() )
        return startContinuation();

    // otherwise it's a 0-size put, so we're done
    return commit();
}

bool Akonadi::Append::commit()
{
    Response response;

    DataStore *db = connection()->storageBackend();
    Location l = db->locationByRawMailbox( m_mailbox );
    if ( !l.isValid() )
      return failureResponse( QString("Unknown collection '%1'.").arg( m_mailbox.constData() ) );

    QString mt;
    QByteArray remote_id;
    foreach( QByteArray flag, m_flags ) {
      if ( flag.startsWith( "\\MimeType" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.indexOf( ']', pos1 );
        mt = flag.mid( pos1 + 1, pos2 - pos1 - 1 );
      } else if ( flag.startsWith( "\\RemoteId" ) ) {
        int pos1 = flag.indexOf( '[' );
        int pos2 = flag.indexOf( ']', pos1 );
        remote_id = flag.mid( pos1 + 1, pos2 - pos1 - 1 );
      }
    }
    // standard imap does not know this attribute, so that's mail
    if ( mt.isEmpty() ) mt = "message/rfc822"; 
    MimeType mimeType = db->mimeTypeByName( mt );
    if ( !mimeType.isValid() ) {
      return failureResponse( QString( "Unknown mime type '%1'.").arg( mt ) );
    }
    int itemId = 0;
    bool ok = db->appendPimItem( m_data, mimeType, l, m_dateTime, remote_id, &itemId );
    response.setTag( tag() );
    if ( !ok ) {
        return failureResponse( "Append failed" );
    }

    // set message flags
    ok = db->appendItemFlags( itemId, m_flags );
    // TODO handle failure

    // the message was appended; now we have to update the counts
    const int existsChange = +1;
    const int recentChange = +1;
    int unseenChange = 0;
    if ( !m_flags.contains( "\\Seen" ) )
        unseenChange = +1;
    // int firstUnseen = ?; // can't be updated atomically, so we probably have to
                            // recalculate it each time it's needed
    ok = db->updateLocationCounts( l, existsChange, recentChange, unseenChange );
    // TODO handle failure by removing the message again from the db

    // TODO if the mailbox is currently selected, the normal new message
    //      actions SHOULD occur.  Specifically, the server SHOULD notify the
    //      client immediately via an untagged EXISTS response.

    response.setTag( tag() );
    response.setSuccess();
    response.setString( "Append completed" );
    emit responseAvailable( response );
    deleteLater();
    return true;
}

bool Akonadi::Append::inContinuation( ) const
{
    return m_size > -1;
}

bool Akonadi::Append::allDataRead( ) const
{
    return ( m_size == 0 );
}

bool Akonadi::Append::startContinuation()
{
    Response response;
    response.setContinuation();
    response.setString( "Ready for literal data" );
    emit responseAvailable( response );
    return false;
}
