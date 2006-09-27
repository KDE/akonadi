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

#include <QtCore/QDebug>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate
{
  public:
    QString path;
    DataReference uid;
    QList<QByteArray> fields;
    Item::List items;
};

ItemFetchJob::ItemFetchJob( const QString & path, QObject * parent ) :
    Job( parent ),
    d( new ItemFetchJobPrivate )
{
  d->path = path;
}

ItemFetchJob::ItemFetchJob(const DataReference & ref, QObject * parent) :
    Job( parent ),
    d( new ItemFetchJobPrivate )
{
  d->path = Collection::root();
  d->uid = ref;
}

ItemFetchJob::~ ItemFetchJob( )
{
  delete d;
}

void ItemFetchJob::doStart()
{
  if ( d->uid.isNull() ) { // collection content listing
    CollectionSelectJob *job = new CollectionSelectJob( d->path, this );
    connect( job, SIGNAL(done(Akonadi::Job*)), SLOT(selectDone(Akonadi::Job*)) );
    job->start();
  } else
    startFetchJob();
}

void ItemFetchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
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

      Item *item = new Item( ref );

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

Item::List ItemFetchJob::items() const
{
  return d->items;
}

void ItemFetchJob::selectDone( Job * job )
{
  if ( job->error() ) {
    setError( job->error(), job->errorMessage() );
    emit done( this );
  } else {
    job->deleteLater();
    // the collection is now selected, fetch the message(s)
    startFetchJob();
  }
}

DataReference ItemFetchJob::parseUid( const QList< QByteArray > & fetchResponse )
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

  QString remoteId;
  int rindex = fetchResponse.indexOf( "REMOTEID" );
  if ( rindex >= 0 && fetchResponse.count() > rindex + 1 )
    remoteId = QString::fromLatin1( fetchResponse[ rindex + 1 ] );

  return DataReference( fetchResponse[index + 1].toUInt(), remoteId );
}

void ItemFetchJob::addFetchField(const QByteArray & field)
{
  d->fields.append( field );
}

void ItemFetchJob::parseFlags(const QByteArray & flagData, Item * item)
{
  QList<QByteArray> flags;
  ImapParser::parseParenthesizedList( flagData, flags );
  foreach ( const QByteArray flag, flags ) {
    if ( flag.startsWith( "\\MimeTypes" ) ) {
      int begin = flag.indexOf( '[' ) + 1;
      item->setMimeType( flag.mid( begin, flag.length() - begin - 1 ) );
    } else {
      item->setFlag( flag );
    }
  }
}

void ItemFetchJob::startFetchJob()
{
  QByteArray command = newTag();
  if ( d->uid.isNull() )
    command += " FETCH 1:* (UID REMOTEID FLAGS";
  else
    command += " UID FETCH " + QByteArray::number( d->uid.persistanceID() ) + " (UID REMOTEID FLAGS RFC822";
  foreach ( QByteArray f, d->fields )
    command += ' ' + f;
  command += ')';
  writeData( command );
}

#include "itemfetchjob.moc"
