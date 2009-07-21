/*
    Copyright (c) 2008 Stephen Kelly <steveire@gmail.com>

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

#ifndef ENTITYTREEMODELPRIVATE_H
#define ENTITYTREEMODELPRIVATE_H

#include <akonadi/item.h>
#include <KJob>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/itemfetchscope.h>
#include <akonadi/mimetypechecker.h>

#include "entitytreemodel.h"

struct Node
{
  Akonadi::Entity::Id id;
  Akonadi::Entity::Id parent;

  enum Type
  {
    Item,
    Collection
  };

  int type;
};

namespace Akonadi
{
/**
 * @internal
 */
class EntityTreeModelPrivate
{
public:

  EntityTreeModelPrivate( EntityTreeModel *parent );
  EntityTreeModel *q_ptr;

//   void collectionStatisticsChanged( Collection::Id, const Akonadi::CollectionStatistics& );

  enum RetrieveDepth {
    Base,
    Recursive
  };

  void fetchCollections( const Collection &collection, CollectionFetchJob::Type = CollectionFetchJob::FirstLevel );
  void fetchItems( const Collection &collection );
  void collectionsFetched( const Akonadi::Collection::List& );
//   void resourceTopCollectionsFetched( const Akonadi::Collection::List& );
  void itemsFetched( const Akonadi::Item::List& );

  void monitoredCollectionAdded( const Akonadi::Collection&, const Akonadi::Collection& );
  void monitoredCollectionRemoved( const Akonadi::Collection& );
  void monitoredCollectionChanged( const Akonadi::Collection& );
  void monitoredCollectionStatisticsChanged( Akonadi::Collection::Id, const Akonadi::CollectionStatistics& );
  void monitoredCollectionMoved( const Akonadi::Collection&, const Akonadi::Collection&, const Akonadi::Collection& );
  void monitoredItemAdded( const Akonadi::Item&, const Akonadi::Collection& );
  void monitoredItemRemoved( const Akonadi::Item& );
  void monitoredItemChanged( const Akonadi::Item&, const QSet<QByteArray>& );
  void monitoredItemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& );

  void monitoredItemLinked( const Akonadi::Item&, const Akonadi::Collection& );
  void monitoredItemUnlinked( const Akonadi::Item&, const Akonadi::Collection& );

  void monitoredMimeTypeChanged( const QString &mimeType, bool monitored );

  Collection getParentCollection( Entity::Id id ) const;
  Collection::List getParentCollections( const Item &item ) const;
  Collection getParentCollection( const Collection &collection ) const;
  Entity::Id childAt( Collection::Id, int position, bool *ok ) const;
  int indexOf( Collection::Id parent, Collection::Id id ) const;
  Item getItem( Item::Id id ) const;
  void removeChildEntities(Collection::Id colId);
  void retrieveAncestors(const Akonadi::Collection& collection);
  void ancestorsFetched(const Akonadi::Collection::List& collectionList);
  void insertCollection(const Akonadi::Collection &collection, const Akonadi::Collection& parent );
  void insertAncestors(const Akonadi::Collection::List &collectionList );
  

  QHash<Collection::Id, Collection> m_collections;
  QHash<Entity::Id, Item> m_items;
  QHash<Collection::Id, QList<Node*> > m_childEntities;
  QSet<Collection::Id> m_populatedCols;
  Collection::List m_ancestors;

  Monitor *m_monitor;
  Collection m_rootCollection;
  Node *m_rootNode;
  QString m_rootCollectionDisplayName;
  QStringList m_mimeTypeFilter;
  MimeTypeChecker m_mimeChecker;
  EntityTreeModel::CollectionFetchStrategy m_collectionFetchStrategy;
  EntityTreeModel::ItemPopulationStrategy m_itemPopulation;
  bool m_includeUnsubscribed;
  bool m_includeStatistics;
  bool m_showRootCollection;

  void startFirstListJob();

  void fetchJobDone( KJob *job );
  void copyJobDone( KJob *job );
  void moveJobDone( KJob *job );
  void updateJobDone( KJob *job );

  /**
   * Returns the index of the node in @p list with the id @p id. Returns -1 if not found.
   */
  int indexOf( const QList<Node*> &list, Entity::Id id ) const;

  /**
   * The id of the collection which starts an item fetch job. This is part of a hack with QObject::sender
   * in itemsReceivedFromJob to correctly insert items into the model.
   */
  static QByteArray ItemFetchCollectionId() {
    return "ItemFetchCollectionId";
  }

  Session *m_session;

  Q_DECLARE_PUBLIC( EntityTreeModel )
};

}

#endif

