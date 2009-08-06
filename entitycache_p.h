/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ENTITYCACHE_P_H
#define AKONADI_ENTITYCACHE_P_H

#include <akonadi/item.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/collection.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>

#include <qobject.h>
#include <QQueue>
#include <QVariant>
#include <QtCore/QQueue>

class KJob;

namespace Akonadi {

/**
  @internal
  QObject part of EntityCache.
*/
class EntityCacheBase : public QObject
{
  Q_OBJECT
  public:
    explicit EntityCacheBase (QObject * parent = 0);

  signals:
    void dataAvailable();

  private slots:
    virtual void fetchResult( KJob* job ) = 0;
};

template <typename T>
struct EntityCacheNode
{
  EntityCacheNode() : pending( false ), invalid( false ) {}
  EntityCacheNode( typename T::Id id ) : entity( T( id ) ), pending( true ), invalid( false ) {}
  T entity;
  bool pending;
  bool invalid;
};

/**
 * @internal
 * A in-memory FIFO cache for a small amount of Entity objects.
 */
template<typename T, typename FetchJob, typename FetchScope>
class EntityCache : public EntityCacheBase
{
  public:
    explicit EntityCache( int maxCapacity, QObject *parent = 0 ) :
      EntityCacheBase( parent ),
      mCapacity( maxCapacity )
    {}

    ~EntityCache()
    {
      qDeleteAll( mCache );
    }

    /** Object is available in the cache and can be retrieved. */
    bool isCached( typename T::Id id ) const
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      return node && !node->pending;
    }

    /** Object has been requested but is not yet loaded into the cache or is already available. */
    bool isRequested( typename T::Id id ) const
    {
      return cacheNodeForId( id );
    }

    /** Returns the cached object if available, an empty instance otherwise. */
    T retrieve( typename T::Id id ) const
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      if ( node && !node->pending && !node->invalid )
        return node->entity;
      return T();
    }

    /** Marks the cache entry as invalid, use in case the object has been deleted on the server. */
    void invalidate( typename T::Id id )
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      if ( node )
        node->invalid = true;
    }

    /** Triggers a re-fetching of a cache entry, use if it has changed on the server. */
    void update( typename T::Id id, const FetchScope &scope )
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      if ( node )
        mCache.removeAll( node );
      if ( !node || node->pending )
        request( id, scope );
      delete node;
    }

    /** Requests the object to be cached if it is not yet in the cache. @returns @c true if it was in the cache already. */
    bool ensureCached( typename T::Id id, const FetchScope &scope )
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      if ( !node ) {
        request( id, scope );
        return false;
      }
      return !node->pending;
    }

    /**
      Asks the cache to retrieve @p id. @p request is used as
      a token to indicate which request has been finished in the
      dataAvailable() signal.
    */
    void request( typename T::Id id, const FetchScope &scope )
    {
      Q_ASSERT( !isRequested( id ) );
      shrinkCache();
      EntityCacheNode<T> *node = new EntityCacheNode<T>( id );
      FetchJob* job = createFetchJob( id );
      job->setFetchScope( scope );
      job->setProperty( "EntityCacheNode", QVariant::fromValue<typename T::Id>( id ) );
      connect( job, SIGNAL(result(KJob*)), SLOT(fetchResult(KJob*)) );
      mCache.enqueue( node );
    }

  private:
    EntityCacheNode<T>* cacheNodeForId( typename T::Id id ) const
    {
      for ( typename QQueue<EntityCacheNode<T>*>::const_iterator it = mCache.constBegin(), endIt = mCache.constEnd();
            it != endIt; ++it )
      {
        if ( (*it)->entity.id() == id )
          return *it;
      }
      return 0;
    }

    void fetchResult( KJob* job )
    {
      typename T::Id id = job->property( "EntityCacheNode" ).template value<typename T::Id>();
      EntityCacheNode<T> *node = cacheNodeForId( id );
      if ( !node )
        return; // got replaced in the meantime

      node->pending = false;
      extractResult( node, job );
      if ( node->entity.id() != id ) { // make sure we find this node again if something went wrong here...
        kWarning() << "Something went very wrong...";
        node->entity.setId( id );
        node->invalid = true;
      }
      emit dataAvailable();
    }

    void extractResult( EntityCacheNode<T>* node, KJob* job ) const;

    inline FetchJob* createFetchJob( typename T::Id id )
    {
      return new FetchJob( T( id ), this );
    }

    /** Tries to reduce the cache size until at least one more object fits in. */
    void shrinkCache()
    {
      while ( mCache.size() >= mCapacity && !mCache.first()->pending )
        delete mCache.dequeue();
    }

  private:
    QQueue<EntityCacheNode<T>*> mCache;
    int mCapacity;
};

template<> inline void EntityCache<Collection, CollectionFetchJob, CollectionFetchScope>::extractResult( EntityCacheNode<Collection>* node, KJob *job ) const
{
  CollectionFetchJob* fetch = qobject_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetch );
  if ( fetch->collections().isEmpty() )
    node->entity = Collection();
  else
    node->entity = fetch->collections().first();
}

template<> inline void EntityCache<Item, ItemFetchJob, ItemFetchScope>::extractResult( EntityCacheNode<Item>* node, KJob *job ) const
{
  ItemFetchJob* fetch = qobject_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetch );
  if ( fetch->items().isEmpty() )
    node->entity = Item();
  else
    node->entity = fetch->items().first();
}

template<> inline CollectionFetchJob* EntityCache<Collection, CollectionFetchJob, CollectionFetchScope>::createFetchJob( Collection::Id id )
{
  return new CollectionFetchJob( Collection( id ), CollectionFetchJob::Base, this );
}

typedef EntityCache<Collection, CollectionFetchJob, CollectionFetchScope> CollectionCache;
typedef EntityCache<Item, ItemFetchJob, ItemFetchScope> ItemCache;

}

#endif
