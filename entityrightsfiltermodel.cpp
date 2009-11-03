/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>


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

#include "entityrightsfiltermodel.h"

#include "entitytreemodel.h"

#include <kdebug.h>

using namespace Akonadi;

namespace Akonadi {

/**
 * @internal
 */
class EntityRightsFilterModelPrivate
{
  public:
    EntityRightsFilterModelPrivate( EntityRightsFilterModel *parent )
      : q_ptr( parent ), mAccessRights( Collection::AllRights )
    {
    }

    bool rightsMatches( const QModelIndex &index ) const
    {
      if ( mAccessRights == Collection::ReadOnly )
        return true;

      const Collection collection = index.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if ( collection.isValid() ) {
        return (mAccessRights & collection.rights());
      } else {
        const Item item = index.data( EntityTreeModel::ItemRole ).value<Item>();
        if ( item.isValid() ) {
          const Collection collection = index.data( EntityTreeModel::ParentCollectionRole ).value<Collection>();
          return (mAccessRights & collection.rights());
        } else {
          return false;
        }
      }
    }

    Q_DECLARE_PUBLIC(EntityRightsFilterModel)
    EntityRightsFilterModel *q_ptr;

    Collection::Rights mAccessRights;
};

}

EntityRightsFilterModel::EntityRightsFilterModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d_ptr( new EntityRightsFilterModelPrivate( this ) )
{
}

EntityRightsFilterModel::~EntityRightsFilterModel()
{
  delete d_ptr;
}

void EntityRightsFilterModel::setAccessRights( Collection::Rights rights )
{
  Q_D(EntityRightsFilterModel);
  d->mAccessRights = rights;
  invalidateFilter();
}

Collection::Rights EntityRightsFilterModel::accessRights() const
{
  Q_D(const EntityRightsFilterModel);
  return d->mAccessRights;
}

bool EntityRightsFilterModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const
{
  Q_D(const EntityRightsFilterModel);

  const QModelIndex modelIndex = sourceModel()->index( sourceRow, 0, sourceParent );

  // one of our children might be accepted, so accept this row if one of our children are accepted.
  for ( int row = 0 ; row < sourceModel()->rowCount( modelIndex ); row++ ) {
    if ( filterAcceptsRow( row, modelIndex ) )
      return true;
  }

  return d->rightsMatches( modelIndex );
}

Qt::ItemFlags EntityRightsFilterModel::flags( const QModelIndex &index ) const
{
  Q_D(const EntityRightsFilterModel);

  if ( d->rightsMatches( index ) )
    return QSortFilterProxyModel::flags( index );
  else
    return QSortFilterProxyModel::flags( index ) & ~(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
}

#include "entityrightsfiltermodel.moc"
