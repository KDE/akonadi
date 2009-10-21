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

#ifndef AKONADI_COMPOUNDFETCHJOB_P_H
#define AKONADI_COMPOUNDFETCHJOB_P_H

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>

#include <kjob.h>

namespace Akonadi {

class CompoundFetchJob : public KJob
{
  Q_OBJECT

  public:
    CompoundFetchJob( const Item &item, const Collection &collection, QObject *parent )
      : KJob( parent ), mItemFetched( false ), mCollectionFetched( false )
    {
      ItemFetchJob *itemFetchJob = new ItemFetchJob( item, this );
      itemFetchJob->fetchScope().fetchFullPayload();

      CollectionFetchJob *collectionFetchJob = new CollectionFetchJob( collection, CollectionFetchJob::Base, this );

      connect( itemFetchJob, SIGNAL( result( KJob* ) ), SLOT( itemFetchResult( KJob* ) ) );
      connect( collectionFetchJob, SIGNAL( result( KJob* ) ), SLOT( collectionFetchResult( KJob* ) ) );

    }

    virtual void start()
    {
    }

    Item item() const
    {
      return mItem;
    }

    Collection collection() const
    {
      return mCollection;
    }

  private Q_SLOTS:
    void itemFetchResult( KJob *job )
    {
      if ( job->error() ) {
        setError( UserDefinedError );
        setErrorText( job->errorText() );
        emitResult();
        return;
      }

      ItemFetchJob *fetchJob = qobject_cast<ItemFetchJob*>( job );
      if ( fetchJob->items().isEmpty() ) {
        setError( UserDefinedError );
        setErrorText( QLatin1String( "Item not found" ) );
        emitResult();
        return;
      }

      mItem = fetchJob->items().first();
      mItemFetched = true;

      if ( mItemFetched && mCollectionFetched )
        
        emitResult();
    }

    void collectionFetchResult( KJob *job )
    {
      if ( job->error() ) {
        setError( UserDefinedError );
        setErrorText( job->errorText() );
        emitResult();
        return;
      }

      CollectionFetchJob *fetchJob = qobject_cast<CollectionFetchJob*>( job );
      if ( fetchJob->collections().isEmpty() ) {
        setError( UserDefinedError );
        setErrorText( QLatin1String( "Collection not found" ) );
        emitResult();
        return;
      }

      mCollection = fetchJob->collections().first();
      mCollectionFetched = true;

      if ( mItemFetched && mCollectionFetched )
        emitResult();
    }

  private:
    bool mItemFetched;
    bool mCollectionFetched;
    Item mItem;
    Collection mCollection;
};

}

#endif
