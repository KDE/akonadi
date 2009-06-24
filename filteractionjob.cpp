/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "filteractionjob.h"

#include "collection.h"
#include "itemfetchjob.h"
#include "itemfetchscope.h"
#include "job_p.h"

#include <KDebug>

using namespace Akonadi;

class Akonadi::FilterActionJob::Private
{
  public:
    Private( FilterActionJob *qq )
      : q( qq )
      , functor( 0 )
    {
    }
    
    ~Private()
    {
      delete functor;
    }

    FilterActionJob *q;
    Collection collection;
    Item::List items;
    FilterAction *functor;
    ItemFetchScope fetchScope;

    // slots:
    void fetchResult( KJob *job );

    void traverseItems();
};

void FilterActionJob::Private::fetchResult( KJob *job )
{
  if ( job->error() ) {
    // KCompositeJob takes care of errors.
    return;
  }

  ItemFetchJob *fjob = dynamic_cast<ItemFetchJob*>( job );
  Q_ASSERT( fjob );
  Q_ASSERT( items.isEmpty() );
  items = fjob->items();
  traverseItems();
}

void FilterActionJob::Private::traverseItems()
{
  Q_ASSERT( functor );
  kDebug() << "Traversing" << items.count() << "items.";
  foreach( const Item &item, items ) {
    if( functor->itemAccepted( item ) ) {
      q->addSubjob( functor->itemAction( item ) );
      kDebug() << "Added subjob for item" << item.id();
    }
  }
  if( q->subjobs().isEmpty() ) {
    kDebug() << "No subjobs; I am done.";
    q->emitResult();
  } else {
    kDebug() << "Have subjobs; calling commit().";
    q->commit();
  }
}



FilterAction::~FilterAction()
{
}



FilterActionJob::FilterActionJob( const Item &item, FilterAction *functor, QObject *parent )
  : TransactionSequence( parent )
  , d( new Private( this ) )
{
  d->functor = functor;
  d->items << item;
}

FilterActionJob::FilterActionJob( const Item::List &items, FilterAction *functor, QObject *parent )
  : TransactionSequence( parent )
  , d( new Private( this ) )
{
  d->functor = functor;
  d->items = items;
}

FilterActionJob::FilterActionJob( const Collection &collection, FilterAction *functor, QObject *parent )
  : TransactionSequence( parent )
  , d( new Private( this ) )
{
  d->functor = functor;
  Q_ASSERT( collection.isValid() );
  d->collection = collection;
}

FilterActionJob::~FilterActionJob()
{
  delete d;
}

#if 0
Item::List FilterActionJob::items()
{
  return d->items;
}
#endif

void FilterActionJob::doStart()
{
  if( d->collection.isValid() ) {
    kDebug() << "Fetching collection" << d->collection.id();
    ItemFetchJob *fjob = new ItemFetchJob( d->collection, this );
    Q_ASSERT( d->functor );
    d->fetchScope = d->functor->fetchScope();
    fjob->setFetchScope( d->fetchScope );
    connect( fjob, SIGNAL(result(KJob*)), this, SLOT(fetchResult(KJob*)) );
  } else {
    d->traverseItems();
  }
}

#include "filteractionjob.moc"
