/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "itemsearchjob.h"

#include "imapparser_p.h"
#include "itemfetchscope.h"
#include "job_p.h"
#include "protocolhelper_p.h"

#include <QtCore/QTimer>

using namespace Akonadi;

class Akonadi::ItemSearchJobPrivate : public JobPrivate
{
  public:
    ItemSearchJobPrivate( ItemSearchJob *parent, const QString &query )
      : JobPrivate( parent ), mQuery( query ), mEmitTimer( 0 )
    {
    }

    void timeout()
    {
      Q_Q( Akonadi::ItemSearchJob );

      mEmitTimer->stop(); // in case we are called by result()
      if ( !mPendingItems.isEmpty() ) {
        if ( !q->error() )
          emit q->itemsReceived( mPendingItems );
        mPendingItems.clear();
      }
    }

    Q_DECLARE_PUBLIC( ItemSearchJob )

    QString mQuery;
    Item::List mItems;
    ItemFetchScope mFetchScope;
    Item::List mPendingItems; // items pending for emitting itemsReceived()
    QTimer* mEmitTimer;
};

ItemSearchJob::ItemSearchJob( const QString & query, QObject * parent )
  : Job( new ItemSearchJobPrivate( this, query ), parent )
{
  Q_D( ItemSearchJob );

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );
}

ItemSearchJob::~ItemSearchJob()
{
}

void ItemSearchJob::setQuery( const QString &query )
{
  Q_D( ItemSearchJob );

  d->mQuery = query;
}

void ItemSearchJob::setFetchScope( const ItemFetchScope &fetchScope )
{
  Q_D( ItemSearchJob );

  d->mFetchScope = fetchScope;
}

ItemFetchScope &ItemSearchJob::fetchScope()
{
  Q_D( ItemSearchJob );

  return d->mFetchScope;
}

void ItemSearchJob::doStart()
{
  Q_D( ItemSearchJob );

  QByteArray command = d->newTag() + " SEARCH ";
  command += ImapParser::quote( d->mQuery.toUtf8() );
  command += ' ' + ProtocolHelper::itemFetchScopeToByteArray( d->mFetchScope );
  command += '\n';
  d->writeData( command );
}

void ItemSearchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( ItemSearchJob );

  if ( tag == "*" ) {
    int begin = data.indexOf( "SEARCH" );
    if ( begin >= 0 ) {

      // split fetch response into key/value pairs
      QList<QByteArray> fetchResponse;
      ImapParser::parseParenthesizedList( data, fetchResponse, begin + 7 );

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

Item::List ItemSearchJob::items() const
{
  Q_D( const ItemSearchJob );

  return d->mItems;
}

QUrl ItemSearchJob::akonadiItemIdUri()
{
  return QUrl( QLatin1String( "http://akonadi-project.org/ontologies/aneo#akonadiItemId" ) );
}

#include "moc_itemsearchjob.cpp"
