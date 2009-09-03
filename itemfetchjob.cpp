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

#include "attributefactory.h"
#include "collection.h"
#include "collectionselectjob_p.h"
#include "imapparser_p.h"
#include "itemfetchscope.h"
#include "job_p.h"
#include "protocol_p.h"
#include "protocolhelper_p.h"

#include <kdebug.h>

#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate : public JobPrivate
{
  public:
    ItemFetchJobPrivate( ItemFetchJob *parent )
      : JobPrivate( parent )
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
    ItemFetchScope mFetchScope;
    Item::List mPendingItems; // items pending for emitting itemsReceived()
    QTimer* mEmitTimer;
};

void ItemFetchJobPrivate::startFetchJob()
{
  QByteArray command = newTag();
  if ( mItem.isValid() )
    command += " " AKONADI_CMD_UID " " AKONADI_CMD_ITEMFETCH " " + QByteArray::number( mItem.id() );
  else if ( !mItem.remoteId().isEmpty() )
    command += " " AKONADI_CMD_RID " " AKONADI_CMD_ITEMFETCH " " + mItem.remoteId().toUtf8();
  else
    command += " " AKONADI_CMD_ITEMFETCH " 1:*";

  command += ProtocolHelper::itemFetchScopeToByteArray( mFetchScope );

  writeData( command );
}

void ItemFetchJobPrivate::selectDone( KJob * job )
{
  if ( !job->error() )
    // the collection is now selected, fetch the message(s)
    startFetchJob();
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

  d->mCollection = Collection::root();
  d->mItem = item;
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

      Item item;
      ProtocolHelper::parseItemFetchResult( fetchResponse, item );
      if ( !item.isValid() )
        return;

      d->mItems.append( item );
      d->mPendingItems.append( item );
      if ( !d->mEmitTimer->isActive() )
        d->mEmitTimer->start();
      return;
    }
  }
  kDebug() << "Unhandled response: " << tag << data;
}

Item::List ItemFetchJob::items() const
{
  Q_D( const ItemFetchJob );

  return d->mItems;
}

void ItemFetchJob::setFetchScope( ItemFetchScope &fetchScope )
{
  Q_D( ItemFetchJob );

  d->mFetchScope = fetchScope;
}

void ItemFetchJob::setFetchScope( const ItemFetchScope &fetchScope )
{
  Q_D( ItemFetchJob );

  d->mFetchScope = fetchScope;
}

ItemFetchScope &ItemFetchJob::fetchScope()
{
  Q_D( ItemFetchJob );

  return d->mFetchScope;
}

Item ItemFetchJob::item() const
{
  Q_D( const ItemFetchJob );

  return d->mItem;
}

Collection ItemFetchJob::collection() const
{
  Q_D( const ItemFetchJob );

  return d->mCollection;
}

void ItemFetchJob::setCollection(const Akonadi::Collection& collection)
{
  Q_D( ItemFetchJob );

  d->mCollection = collection;
}


#include "itemfetchjob.moc"
