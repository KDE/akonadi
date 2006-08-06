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
#include "itemfetchjob.h"

#include <QDebug>

using namespace PIM;

class PIM::ItemFetchJobPrivate
{
  public:
    QByteArray path;
    DataReference uid;
    QList<QByteArray> fields;
    Item::List items;
};

PIM::ItemFetchJob::ItemFetchJob( const QByteArray & path, QObject * parent ) :
    Job( parent ),
    d( new ItemFetchJobPrivate )
{
  d->path = path;
}

PIM::ItemFetchJob::ItemFetchJob(const DataReference & ref, QObject * parent) :
    Job( parent ),
    d( new ItemFetchJobPrivate )
{
  d->path = Collection::root();
  d->uid = ref;
}

PIM::ItemFetchJob::~ ItemFetchJob( )
{
  delete d;
}

void PIM::ItemFetchJob::doStart()
{
  QByteArray selectPath;
  if ( d->uid.isNull() ) // collection content listing
    selectPath = d->path;
  else                   // message fetching
    selectPath = /*Collection::root()*/ "res1/foo"; // ### just until the server is fixed
  CollectionSelectJob *job = new CollectionSelectJob( selectPath, this );
  connect( job, SIGNAL(done(PIM::Job*)), SLOT(selectDone(PIM::Job*)) );
  job->start();
}

void PIM::ItemFetchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
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
        if ( fetch[i] == "FLAGS" )
          parseFlags( fetch[i + 1], item );
        else if ( fetch[i] == "RFC822" ) {
          item->setData( fetch[i + 1] );
        }

      }

      d->items.append( item );
      return;
    }
  }
  qDebug() << "Unhandled response in message fetch job: " << tag << data;
}

PIM::Item::List PIM::ItemFetchJob::items() const
{
  return d->items;
}

void PIM::ItemFetchJob::selectDone( PIM::Job * job )
{
  if ( job->error() ) {
    setError( job->error(), job->errorMessage() );
    emit done( this );
  } else {
    job->deleteLater();
    // the collection is now selected, fetch the message(s)
    QByteArray command = newTag();
    if ( d->uid.isNull() )
      command += " FETCH 1:* (UID FLAGS";
    else
    command += " UID FETCH " + d->uid.persistanceID().toLatin1() + " (UID FLAGS RFC822";
    foreach ( QByteArray f, d->fields )
      command += " " + f;
    command += ")";
    writeData( command );
  }
}

DataReference PIM::ItemFetchJob::parseUid( const QList< QByteArray > & fetchResponse )
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

void PIM::ItemFetchJob::addFetchField(const QByteArray & field)
{
  d->fields.append( field );
}

void PIM::ItemFetchJob::parseFlags(const QByteArray & flagData, Item * item)
{
  QList<QByteArray> flags;
  ImapParser::parseParenthesizedList( flagData, flags );
  foreach ( const QByteArray flag, flags )
    item->setFlag( flag );
}

#include "itemfetchjob.moc"
