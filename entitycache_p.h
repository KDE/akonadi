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
#include <akonadi/session.h>

#include "akonadiprivate_export.h"

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
class AKONADI_TESTS_EXPORT EntityCacheBase : public QObject
{
  Q_OBJECT
  public:
    explicit EntityCacheBase ( Session *session, QObject * parent = 0 );

    void setSession(Session *session);

  protected:
    Session *session;

  signals:
    void dataAvailable();

  private slots:
    virtual void processResult( KJob* job ) = 0;
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
template<typename T, typename FetchJob, typename FetchScope_>
class EntityCache : public EntityCacheBase
{
  public:
    typedef FetchScope_ FetchScope;
    explicit EntityCache( int maxCapacity, Session *session = 0, QObject *parent = 0 ) :
      EntityCacheBase( session, parent ),
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
    virtual T retrieve( typename T::Id id ) const
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      if ( node && !node->pending && !node->invalid ) {
        return node->entity;
      }
      return T();
    }

    /** Marks the cache entry as invalid, use in case the object has been deleted on the server. */
    void invalidate( typename T::Id id )
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      if ( node ) {
        node->invalid = true;
      }
    }

    /** Triggers a re-fetching of a cache entry, use if it has changed on the server. */
    void update( typename T::Id id, const FetchScope &scope )
    {
      EntityCacheNode<T>* node = cacheNodeForId( id );
      if ( node ) {
        mCache.removeAll( node );
        if ( node->pending ) {
          request( id, scope );
        }
        delete node;
      }
    }

    /** Requests the object to be cached if it is not yet in the cache. @returns @c true if it was in the cache already. */
    virtual bool ensureCached( typename T::Id id, const FetchScope &scope )
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
    virtual void request( typename T::Id id, const FetchScope &scope )
    {
      Q_ASSERT( !isRequested( id ) );
      shrinkCache();
      EntityCacheNode<T> *node = new EntityCacheNode<T>( id );
      FetchJob* job = createFetchJob( id );
      job->setFetchScope( scope );
      job->setProperty( "EntityCacheNode", QVariant::fromValue<typename T::Id>( id ) );
      connect( job, SIGNAL( result( KJob* )), SLOT(processResult( KJob* ) ) );
      mCache.enqueue( node );
    }

  private:
    EntityCacheNode<T>* cacheNodeForId( typename T::Id id ) const
    {
      for ( typename QQueue<EntityCacheNode<T>*>::const_iterator it = mCache.constBegin(), endIt = mCache.constEnd();
            it != endIt; ++it ) {
        if ( ( *it )->entity.id() == id ) {
          return *it;
        }
      }
      return 0;
    }

    void processResult( KJob* job )
    {
      // Error handling?
      typename T::Id id = job->property( "EntityCacheNode" ).template value<typename T::Id>();
      EntityCacheNode<T> *node = cacheNodeForId( id );
      if ( !node ) {
        return; // got replaced in the meantime
      }

      node->pending = false;
      extractResult( node, job );
      // make sure we find this node again if something went wrong here,
      // most likely the object got deleted from the server in the meantime
      if ( node->entity.id() != id ) {
        // TODO: Recursion guard? If this is called with non-existing ids, the if will never be true!
        node->entity.setId( id );
        node->invalid = true;
      }
      emit dataAvailable();
    }

    void extractResult( EntityCacheNode<T>* node, KJob* job ) const;

    inline FetchJob* createFetchJob( typename T::Id id )
    {
      return new FetchJob( T( id ), session );
    }

    /** Tries to reduce the cache size until at least one more object fits in. */
    void shrinkCache()
    {
      while ( mCache.size() >= mCapacity && !mCache.first()->pending ) {
        delete mCache.dequeue();
      }
    }

  private:
    QQueue<EntityCacheNode<T>*> mCache;
    int mCapacity;
};

template<> inline void EntityCache<Collection, CollectionFetchJob, CollectionFetchScope>::extractResult( EntityCacheNode<Collection>* node, KJob *job ) const
{
  CollectionFetchJob* fetch = qobject_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetch );
  if ( fetch->collections().isEmpty() ) {
    node->entity = Collection();
  } else {
    node->entity = fetch->collections().first();
  }
}

template<> inline void EntityCache<Item, ItemFetchJob, ItemFetchScope>::extractResult( EntityCacheNode<Item>* node, KJob *job ) const
{
  ItemFetchJob* fetch = qobject_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetch );
  if ( fetch->items().isEmpty() ) {
    node->entity = Item();
  } else {
    node->entity = fetch->items().first();
  }
}

template<> inline CollectionFetchJob* EntityCache<Collection, CollectionFetchJob, CollectionFetchScope>::createFetchJob( Collection::Id id )
{
  return new CollectionFetchJob( Collection( id ), CollectionFetchJob::Base, session );
}

typedef EntityCache<Collection, CollectionFetchJob, CollectionFetchScope> CollectionCache;
typedef EntityCache<Item, ItemFetchJob, ItemFetchScope> ItemCache;


template<typename T>
class Comparator
{
public:
  static bool compare(const typename T::List &lhs_, const QList<typename T::Id> &rhs_ )
  {
    if ( lhs_.size() != rhs_.size() ) {
      return false;
    }

    typename T::List lhs = lhs_;
    QList<typename T::Id> rhs = rhs_;

    qSort( lhs );
    qSort( rhs );
    for (int i = 0; i < lhs.size(); ++i) {
      if (lhs.at(i).id() != rhs.at(i)) {
        return false;
      }
    }
    return true;
  }

  static bool compare(const QList<typename T::Id> &l1, const typename T::List &l2)
  {
    return compare( l2, l1 );
  }

  static bool compare(const typename T::List &l1, const typename T::List &l2)
  {
    typename T::List l1_ = l1;
    typename T::List l2_ = l2;
    qSort( l1_ );
    qSort( l2_ );
    return l1_ == l2_;
  }
};


template <typename T>
struct EntityListCacheNode
{
  EntityListCacheNode( const typename T::List &list ) : entityList( list ), pending( true ), invalid( false ) {}
  EntityListCacheNode( const QList<typename T::Id> &list ) : pending( true ), invalid( false ) {
    foreach ( typename T::Id id, list ) {
      entityList.append( T( id ) );
    }
  }
  typename T::List entityList;
  bool pending;
  bool invalid;
};

template<typename T, typename FetchJob, typename FetchScope_>
class EntityListCache : public EntityCacheBase
{
public:
  typedef FetchScope_ FetchScope;

  explicit EntityListCache( int maxCapacity, Session *session = 0, QObject *parent = 0 ) :
    EntityCacheBase( session, parent ),
    mCapacity( maxCapacity )
  {}

  ~EntityListCache()
  {
    qDeleteAll( mCache );
  }

  /** Returns the cached object if available, an empty instance otherwise. */
  template<typename TArg>
  typename T::List retrieve( const QList<TArg> &id ) const
  {
    EntityListCacheNode<T>* node = cacheNodeForId( id );
    if ( node && !node->pending && !node->invalid ) {
      return node->entityList;
    }
    return typename T::List();
  }

  /** Requests the object to be cached if it is not yet in the cache. @returns @c true if it was in the cache already. */
  template<typename TArg>
  bool ensureCached( const QList<TArg> &id, const FetchScope &scope )
  {
    EntityListCacheNode<T>* node = cacheNodeForId( id );
    if ( !node ) {
      request( id, scope );
      return false;
    }
    return !node->pending;
  }

  /** Marks the cache entry as invalid, use in case the object has been deleted on the server. */
  template<typename TArg>
  void invalidate( const QList<TArg> &id )
  {
    EntityListCacheNode<T>* node = cacheNodeForId( id );
    if ( node ) {
      node->invalid = true;
    }
  }

  /** Triggers a re-fetching of a cache entry, use if it has changed on the server. */
  template<typename TArg>
  void update( const QList<TArg> &id, const FetchScope &scope )
  {
    EntityListCacheNode<T>* node = cacheNodeForId( id );
    if ( node ) {
      mCache.removeAll( node );
      if ( node->pending ) {
        request( id, scope );
      }
      delete node;
    }
  }

  /**
    Asks the cache to retrieve @p id. @p request is used as
    a token to indicate which request has been finished in the
    dataAvailable() signal.
  */
  template<typename TArg>
  void request( const QList<TArg> &id, const FetchScope &scope )
  {
    Q_ASSERT( !isRequested( id ) );
    shrinkCache();
    EntityListCacheNode<T> *node = new EntityListCacheNode<T>( id );
    FetchJob* job = createFetchJob( id );
    job->setFetchScope( scope );
    job->setProperty( "EntityListCacheNode", QVariant::fromValue<typename T::List>( getTList( id ) ) );
    connect( job, SIGNAL(result(KJob*)), SLOT(processResult(KJob*)) );
    mCache.enqueue( node );
  }

  /** Object has been requested but is not yet loaded into the cache or is already available. */
  template<typename TArg>
  bool isRequested( const QList<TArg> &id ) const
  {
    return cacheNodeForId( id );
  }

  /** Object is available in the cache and can be retrieved. */
  template<typename TArg>
  bool isCached( const QList<TArg> &id ) const
  {
    EntityListCacheNode<T>* node = cacheNodeForId( id );
    return node && !node->pending;
  }

private:

  typename T::List getTList( const QList<typename T::Id> &id )
  {
    typename T::List ids;
    foreach ( typename T::Id id_, id ) {
      ids.append( T( id_ ) );
    }
    return ids;
  }

  typename T::List getTList( const typename T::List &id )
  {
    return id;
  }

  /** Tries to reduce the cache size until at least one more object fits in. */
  void shrinkCache()
  {
    while ( mCache.size() >= mCapacity && !mCache.first()->pending ) {
      delete mCache.dequeue();
    }
  }

  template<typename TArg>
  EntityListCacheNode<T>* cacheNodeForId( const QList<TArg> &id ) const
  {
    for ( typename QQueue<EntityListCacheNode<T>*>::const_iterator it = mCache.constBegin(), endIt = mCache.constEnd();
          it != endIt; ++it ) {
      if ( Comparator<T>::compare( ( *it )->entityList, id ) ) {
        return *it;
      }
    }
    return 0;
  }

  template<typename TArg>
  inline FetchJob* createFetchJob( const QList<TArg> &id )
  {
    return new FetchJob( id, session );
  }

  void processResult( KJob* job )
  {
    typename T::List ids = job->property( "EntityListCacheNode" ).template value<typename T::List>();

    EntityListCacheNode<T> *node = cacheNodeForId( ids );
    if ( !node ) {
      return; // got replaced in the meantime
    }

    node->pending = false;
    extractResult( node, job );
    // make sure we find this node again if something went wrong here,
    // most likely the object got deleted from the server in the meantime
    if ( node->entityList != ids ) {
      node->entityList = ids;
      node->invalid = true;
    }
    emit dataAvailable();
  }

  void extractResult( EntityListCacheNode<T>* node, KJob* job ) const;


private:
  QQueue<EntityListCacheNode<T>*> mCache;
  int mCapacity;
};

template<> inline void EntityListCache<Collection, CollectionFetchJob, CollectionFetchScope>::extractResult( EntityListCacheNode<Collection>* node, KJob *job ) const
{
  CollectionFetchJob* fetch = qobject_cast<CollectionFetchJob*>( job );
  Q_ASSERT( fetch );
  node->entityList = fetch->collections();
}

template<> inline void EntityListCache<Item, ItemFetchJob, ItemFetchScope>::extractResult( EntityListCacheNode<Item>* node, KJob *job ) const
{
  ItemFetchJob* fetch = qobject_cast<ItemFetchJob*>( job );
  Q_ASSERT( fetch );
  node->entityList = fetch->items();
}

template<>
template<typename TArg>
inline CollectionFetchJob* EntityListCache<Collection, CollectionFetchJob, CollectionFetchScope>::createFetchJob( const QList<TArg> &id )
{
  return new CollectionFetchJob( id, CollectionFetchJob::Base, session );
}

typedef EntityListCache<Collection, CollectionFetchJob, CollectionFetchScope> CollectionListCache;
typedef EntityListCache<Item, ItemFetchJob, ItemFetchScope> ItemListCache;

}

#endif
