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
#include "session_p.h"

#include <kdebug.h>
#include <KLocalizedString>

#include <QtCore/QStringList>
#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate : public JobPrivate
{
  public:
    ItemFetchJobPrivate( ItemFetchJob *parent )
      : JobPrivate( parent ),
        mEmitTimer( 0 ),
        mValuePool( 0 )
    {
      mCollection = Collection::root();
    }

    ~ItemFetchJobPrivate()
    {
      delete mValuePool;
    }

    void init()
    {
      Q_Q( ItemFetchJob );
      mEmitTimer = new QTimer( q );
      mEmitTimer->setSingleShot( true );
      mEmitTimer->setInterval( 100 );
      q->connect( mEmitTimer, SIGNAL(timeout()), q, SLOT(timeout()) );
      q->connect( q, SIGNAL(result(KJob*)), q, SLOT(timeout()) );
    }

    void timeout()
    {
      Q_Q( ItemFetchJob );

      mEmitTimer->stop(); // in case we are called by result()
      if ( !mPendingItems.isEmpty() ) {
        if ( !q->error() )
          emit q->itemsReceived( mPendingItems );
        mPendingItems.clear();
      }
    }

    void startFetchJob();
    void selectDone( KJob * job );

    QString jobDebuggingString() const /*Q_DECL_OVERRIDE*/ {
      if ( mRequestedItems.isEmpty() ) {
        return QString::fromLatin1( "All items from collection %1" ).arg( mCollection.id() );
      } else {
        try {
          return QString::fromLatin1( ProtocolHelper::entitySetToByteArray( mRequestedItems, AKONADI_CMD_ITEMFETCH ) );
        } catch ( const Exception &e ) {
          return QString::fromUtf8( e.what() );
        }
      }
    }

    Q_DECLARE_PUBLIC( ItemFetchJob )

    Collection mCollection;
    Item::List mRequestedItems;
    Item::List mResultItems;
    ItemFetchScope mFetchScope;
    Item::List mPendingItems; // items pending for emitting itemsReceived()
    QTimer* mEmitTimer;
    ProtocolHelperValuePool *mValuePool;
};

void ItemFetchJobPrivate::startFetchJob()
{
  Q_Q( ItemFetchJob );
  QByteArray command = newTag();
  if ( mRequestedItems.isEmpty() ) {
    command += " " AKONADI_CMD_ITEMFETCH " 1:*";
  } else {
    try {
      command += ProtocolHelper::entitySetToByteArray( mRequestedItems, AKONADI_CMD_ITEMFETCH );
    } catch ( const Exception &e ) {
      q->setError( Job::Unknown );
      q->setErrorText( QString::fromUtf8( e.what() ) );
      q->emitResult();
      return;
    }
  }

  //This is only required for 4.10
  if ( protocolVersion() < 30 ) {
    if ( mFetchScope.ignoreRetrievalErrors() ) {
      kDebug() << "IGNOREERRORS is not available with this akonadi protocol version";
    }
    mFetchScope.setIgnoreRetrievalErrors( false );
  }
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

  d->init();
  d->mCollection = collection;
  d->mValuePool = new ProtocolHelperValuePool; // only worth it for lots of results
}

ItemFetchJob::ItemFetchJob( const Item & item, QObject * parent)
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->init();
  d->mRequestedItems.append( item );
}

ItemFetchJob::ItemFetchJob(const Akonadi::Item::List& items, QObject* parent)
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->init();
  d->mRequestedItems = items;
}

ItemFetchJob::ItemFetchJob(const QList<Akonadi::Item::Id>& items, QObject* parent)
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->init();
  foreach(Item::Id id, items)
    d->mRequestedItems.append(Item(id));
}

ItemFetchJob::~ItemFetchJob()
{
}

void ItemFetchJob::doStart()
{
  Q_D( ItemFetchJob );

  if ( d->mRequestedItems.isEmpty() ) { // collection content listing
    if ( d->mCollection == Collection::root() ) {
      setErrorText( i18n( "Cannot list root collection." ) );
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
      ProtocolHelper::parseItemFetchResult( fetchResponse, item, d->mValuePool );
      if ( !item.isValid() )
        return;

      d->mResultItems.append( item );
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

  return d->mResultItems;
}

void ItemFetchJob::clearItems()
{
  Q_D( ItemFetchJob );

  d->mResultItems.clear();
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

void ItemFetchJob::setCollection(const Akonadi::Collection& collection)
{
  Q_D( ItemFetchJob );

  d->mCollection = collection;
}

#include "moc_itemfetchjob.cpp"
