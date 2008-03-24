/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemfetchjob.h"

#include "collection.h"
#include "collectionselectjob.h"
#include "imapparser_p.h"
#include "itemserializer.h"
#include "itemserializerplugin.h"
#include "job_p.h"
#include "entity_p.h"

#include <kdebug.h>

#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate : public JobPrivate
{
  public:
    ItemFetchJobPrivate( ItemFetchJob *parent )
      : JobPrivate( parent ),
        mFetchAllParts( false )
    {
    }

    void timeout()
    {
      Q_Q( ItemFetchJob );

      mEmitTimer->stop(); // in case we are called by result()
      if ( !mPendingItems.isEmpty() ) {
        emit q->itemsReceived( mPendingItems );
        mPendingItems.clear();
      }
    }

    void startFetchJob();
    void selectDone( KJob * job );

    Q_DECLARE_PUBLIC( ItemFetchJob )

    Collection mCollection;
    Item mItem;
    Item::List mItems;
    QStringList mFetchParts;
    bool mFetchAllParts;
    Item::List mPendingItems; // items pending for emitting itemsReceived()
    QTimer* mEmitTimer;
};

void ItemFetchJobPrivate::startFetchJob()
{
  Q_Q( ItemFetchJob );

  QByteArray command = newTag();
  if ( !mItem.isValid() )
    command += " FETCH 1:*";
  else
    command += " UID FETCH " + QByteArray::number( mItem.id() );

  if ( !mFetchAllParts ) {
    command += " (UID REMOTEID FLAGS";
    foreach ( QString part, mFetchParts ) {
      command += ' ' + part.toUtf8();
    }
    command += ")\n";
  } else {
    command += " AKALL\n";
  }
  writeData( command );
}

void ItemFetchJobPrivate::selectDone( KJob * job )
{
  if ( !job->error() )
    // the collection is now selected, fetch the message(s)
    startFetchJob();
}

ItemFetchJob::ItemFetchJob( QObject *parent )
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );
}

ItemFetchJob::ItemFetchJob( const Collection &collection, QObject * parent )
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );

  d->mCollection = collection;
}

ItemFetchJob::ItemFetchJob( const Item & item, QObject * parent)
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );

  setItem( item );
}

ItemFetchJob::~ItemFetchJob()
{
}

void ItemFetchJob::doStart()
{
  Q_D( ItemFetchJob );

  if ( !d->mItem.isValid() ) { // collection content listing
    if ( d->mCollection == Collection::root() ) {
      setErrorText( QLatin1String("Cannot list root collection.") );
      setError( Unknown );
      emitResult();
    }
    CollectionSelectJob *job = new CollectionSelectJob( d->mCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(selectDone(KJob*)) );
    addSubjob( job );
  } else
    d->startFetchJob();
}

void ItemFetchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( ItemFetchJob );

  if ( tag == "*" ) {
    int begin = data.indexOf( "FETCH" );
    if ( begin >= 0 ) {

      // split fetch response into key/value pairs
      QList<QByteArray> fetchResponse;
      ImapParser::parseParenthesizedList( data, fetchResponse, begin + 6 );

      // create a new item object
      Item::Id uid = -1;
      int rev = -1;
      QString rid;
      QString mimeType;

      for ( int i = 0; i < fetchResponse.count() - 1; i += 2 ) {
        const QByteArray key = fetchResponse.value( i );
        const QByteArray value = fetchResponse.value( i + 1 );

        if ( key == "UID" )
          uid = value.toLongLong();
        else if ( key == "REV" )
          rev = value.toInt();
        else if ( key == "REMOTEID" )
          rid = QString::fromUtf8( value );
        else if ( key == "MIMETYPE" )
          mimeType = QString::fromLatin1( value );
      }

      if ( uid < 0 || rev < 0 || mimeType.isEmpty() ) {
        kWarning( 5250 ) << "Broken fetch response: UID, RID, REV or MIMETYPE missing!";
        return;
      }

      Item item( uid );
      item.setRemoteId( rid );
      item.setRevision( rev );
      item.setMimeType( mimeType );
      if ( !item.isValid() )
        return;

      // parse fetch response fields
      for ( int i = 0; i < fetchResponse.count() - 1; i += 2 ) {
        const QByteArray key = fetchResponse.value( i );
        // skip stuff we dealt with already
        if ( key == "UID" || key == "REV" || key == "REMOTEID" || key == "MIMETYPE" )
          continue;
        // flags
        if ( key == "FLAGS" ) {
          QList<QByteArray> flags;
          ImapParser::parseParenthesizedList( fetchResponse[i + 1], flags );
          foreach ( const QByteArray flag, flags ) {
            item.setFlag( flag );
          }
        } else {
          ItemSerializer::deserialize( item, QString::fromLatin1( key ), fetchResponse.value( i + 1 ) );
        }
      }

      item.d_ptr->resetChangeLog();
      d->mItems.append( item );
      d->mPendingItems.append( item );
      if ( !d->mEmitTimer->isActive() )
        d->mEmitTimer->start();
      return;
    }
  }
  kDebug( 5250 ) << "Unhandled response: " << tag << data;
}

Item::List ItemFetchJob::items() const
{
  Q_D( const ItemFetchJob );

  return d->mItems;
}

void ItemFetchJob::setCollection(const Collection &collection)
{
  Q_D( ItemFetchJob );

  d->mCollection = collection;
  d->mItem = Item();
}

void Akonadi::ItemFetchJob::setItem(const Item & item)
{
  Q_D( ItemFetchJob );

  d->mCollection = Collection::root();
  d->mItem = item;
}

void ItemFetchJob::addFetchPart( const QString &identifier )
{
  Q_D( ItemFetchJob );

  if ( !d->mFetchParts.contains( identifier ) )
    d->mFetchParts.append( identifier );
}

void ItemFetchJob::fetchAllParts()
{
  Q_D( ItemFetchJob );

  d->mFetchAllParts = true;
}

#include "itemfetchjob.moc"
