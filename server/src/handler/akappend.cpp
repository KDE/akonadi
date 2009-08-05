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

#include "libs/imapparser_p.h"
#include "imapstreamparser.h"

#include "append.h"
#include "akappend.h"
#include "response.h"
#include "handlerhelper.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"

#include <QtCore/QDebug>

using namespace Akonadi;

AkAppend::AkAppend()
    : Handler()
{
}

AkAppend::~AkAppend()
{
}

bool Akonadi::AkAppend::commit()
{
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

    bool ok = db->appendPimItem( m_parts, mimeType, col, m_dateTime, remote_id, item );

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

bool AkAppend::parseStream()
{
    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        (partname literal)+
    //
    // Syntax:
    // x-akappend = "X-AKAPPEND" SP mailbox SP size [SP flag-list] [SP date-time] SP (partname SP literal)+

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
    // FIXME Should we return an error if m_dateTime is invalid?
  }
  // if date/time is not given then it will be set to the current date/time
  // by the database

  // parse part specification
  QList<QPair<QByteArray, QPair<qint64, int> > > partSpecs;
  QByteArray partName = "";
  qint64 partSize = -1;
  qint64 partSizes = 0;
  bool ok = false;

  QList<QByteArray> list = m_streamParser->readParenthesizedList();
  Q_FOREACH( const QByteArray &item, list )
  {
    if (partName.isEmpty() && partSize == -1)
    {
      partName = item;
      continue;
    }
    if (item.startsWith(':'))
    {
      int pos = 1;
      ImapParser::parseNumber( item, partSize, &ok, pos );
      if( !ok )
        partSize = 0;

      int version = 0;
      QByteArray plainPartName;
      ImapParser::splitVersionedKey( partName, plainPartName, version );

      partSpecs.append( qMakePair( plainPartName, qMakePair( partSize, version ) ) );
      partName = "";
      partSizes += partSize;
      partSize = -1;
    }
  }

  m_size = qMax( partSizes, m_size );

  // TODO streaming support!
  QByteArray allParts = m_streamParser->readString();

  // chop up literal data in parts
  int pos = 0; // traverse through part data now
  QPair<QByteArray, QPair<qint64, int> > partSpec;
  foreach( partSpec, partSpecs ) {
    // wrap data into a part
    Part part;
    part.setName( QLatin1String( partSpec.first ) );
    part.setData( allParts.mid( pos, partSpec.second.first ) );
    if ( partSpec.second.second != 0 )
      part.setVersion( partSpec.second.second );
    m_parts.append( part );
    pos += partSpec.second.first;
  }

  return commit();
}

#include "akappend.moc"
