/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "collection.h"
#include "collectionselectjob.h"
#include "imapparser.h"
#include "messagefetchjob.h"

#include <kmime/kmime_message.h>
#include <kmime/kmime_headers.h>

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::MessageFetchJobPrivate
{
  public:
    Message::List messages;

  public:
    template <typename T> void parseAddrList( const QList<QByteArray> &addrList, T *hdr )
    {
      for ( QList<QByteArray>::ConstIterator it = addrList.constBegin(); it != addrList.constEnd(); ++it ) {
        QList<QByteArray> addr;
        ImapParser::parseParenthesizedList( *it, addr );
        if ( addr.count() != 4 ) {
          qWarning() << "Error parsing envelope address field: " << addr;
          continue;
        }
        KMime::Types::Mailbox addrField;
        addrField.setNameFrom7Bit( addr[0] );
        addrField.setAddress( addr[2] + '@' + addr[3] );
        hdr->addAddress( addrField );
      }
    }
};

MessageFetchJob::MessageFetchJob( const QString & path, QObject * parent ) :
    ItemFetchJob( path, parent ),
    d( new MessageFetchJobPrivate )
{
  addFetchField( "ENVELOPE" );
}

MessageFetchJob::MessageFetchJob( const DataReference & ref, QObject * parent ) :
    ItemFetchJob( ref, parent ),
    d( new MessageFetchJobPrivate )
{
}

MessageFetchJob::~MessageFetchJob()
{
  delete d;
}

void MessageFetchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  if ( tag == "*" ) {
    int begin = data.indexOf( "FETCH" );
    if ( begin >= 0 ) {

      // split fetch response into key/value pairs
      QList<QByteArray> fetch;
      ImapParser::parseParenthesizedList( data, fetch, begin + 6 );

      // create a new message object
      DataReference ref = parseUid( fetch );
      if ( ref.isNull() ) {
        qWarning() << "No UID found in fetch response - skipping";
        return;
      }
      Message* msg = new Message( ref );
      KMime::Message* mime = new KMime::Message();
      msg->setMime( mime );
      d->messages.append( msg );

      // parse fetch response fields
      for ( int i = 0; i < fetch.count() - 1; i += 2 ) {
        // flags
        if ( fetch[i] == "FLAGS" )
          parseFlags( fetch[i + 1], msg );
        // envelope
        else if ( fetch[i] == "ENVELOPE" ) {
          QList<QByteArray> env;
          ImapParser::parseParenthesizedList( fetch[i + 1], env );
          Q_ASSERT( env.count() >= 10 );
          // date
          mime->date()->from7BitString( env[0] );
          // subject
          mime->subject()->from7BitString( env[1] );
          // from
          QList<QByteArray> addrList;
          ImapParser::parseParenthesizedList( env[2], addrList );
          if ( !addrList.isEmpty() )
            d->parseAddrList( addrList, mime->from() );
          // sender
          // not supported by kmime, skipp it
          // reply-to
          ImapParser::parseParenthesizedList( env[4], addrList );
          if ( !addrList.isEmpty() )
            d->parseAddrList( addrList, mime->replyTo() );
          // to
          ImapParser::parseParenthesizedList( env[5], addrList );
          if ( !addrList.isEmpty() )
            d->parseAddrList( addrList, mime->to() );
          // cc
          ImapParser::parseParenthesizedList( env[6], addrList );
          if ( !addrList.isEmpty() )
            d->parseAddrList( addrList, mime->cc() );
          // bcc
          ImapParser::parseParenthesizedList( env[7], addrList );
          if ( !addrList.isEmpty() )
            d->parseAddrList( addrList, mime->bcc() );
          // in-reply-to
          mime->inReplyTo()->from7BitString( env[8] );
          // message id
          mime->messageID()->from7BitString( env[9] );
        }
        // rfc822 body
        else if ( fetch[i] == "RFC822" ) {
          mime->setContent( fetch[i + 1] );
        }
      }
      Q_ASSERT( msg );
      return;
    }
  }
  qDebug() << "Unhandled response in message fetch job: " << tag << data;
}

Message::List MessageFetchJob::messages() const
{
  return d->messages;
}

Item::List Akonadi::MessageFetchJob::items() const
{
  Item::List list;
  for ( Message::List::ConstIterator it = d->messages.constBegin(); it != d->messages.constEnd(); ++it )
    list.append( *it );
  return list;
}

#include "messagefetchjob.moc"
