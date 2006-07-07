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
#include "itemlistjob.h"

#include <QDebug>

using namespace PIM;

class PIM::ItemListJobPrivate
{
  public:
    QByteArray path;
    Item::List items;
};

PIM::ItemListJob::ItemListJob( const QByteArray & path, QObject * parent ) :
    Job( parent ),
    d( new ItemListJobPrivate )
{
  d->path = path;
}

PIM::ItemListJob::~ ItemListJob( )
{
  delete d;
}

void PIM::ItemListJob::doStart()
{
  CollectionSelectJob *job = new CollectionSelectJob( d->path, this );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(selectDone(PIM::Job*)) );
  job->start();
}

void PIM::ItemListJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
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
      
      PIM::Item *item = new PIM::Item( ref );

      // parse fetch response fields
      for ( int i = 0; i < fetch.count() - 1; i += 2 ) {
        // flags
        if ( fetch[i] == "FLAGS" ) {
          QList<QByteArray> flags;
          ImapParser::parseParenthesizedList( fetch[i + 1], flags );
          foreach ( const QByteArray flag, flags )
            item->setFlag( flag );
        }
      }

      d->items.append( item );
      return;
    }
  }
  qDebug() << "Unhandled response in message fetch job: " << tag << data;
}

PIM::Item::List PIM::ItemListJob::items() const
{
  return d->items;
}

void PIM::ItemListJob::selectDone( PIM::Job * job )
{
  if ( job->error() ) {
    setError( job->error(), job->errorMessage() );
    emit done( this );
  } else {
    job->deleteLater();
    // the collection is now selected, fetch the message(s)
    QByteArray command = newTag();
    command += " FETCH 1:* (UID FLAGS)";
    writeData( command );
  }
}

DataReference PIM::ItemListJob::parseUid( const QList< QByteArray > & fetchResponse )
{
  int index = fetchResponse.indexOf( "UID" );
  if ( index < 0 ) {
    qWarning() << "Fetch response doesn't contain a UID field!";
    return DataReference();
  }
  if ( index == fetchResponse.count() - 1 ) {
    qWarning() << "Broken fetch response: No value for UID field!";
    return DataReference();
  }
  return DataReference( fetchResponse[index + 1], QString() );
}

#include "itemlistjob.moc"
