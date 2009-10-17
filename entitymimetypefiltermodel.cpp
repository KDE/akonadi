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

#include "entitymimetypefiltermodel.h"

#include "entitytreemodel.h"
#include "mimetypechecker.h"

#include <kdebug.h>
#include <kmimetype.h>

#include <QtCore/QString>
#include <QtCore/QStringList>

using namespace Akonadi;

namespace Akonadi {
/**
 * @internal
 */
class EntityMimeTypeFilterModelPrivate
{
  public:
    EntityMimeTypeFilterModelPrivate( EntityMimeTypeFilterModel *parent )
      : q_ptr( parent ),
        m_headerGroup(EntityTreeModel::EntityTreeHeaders)
    {
    }

    Q_DECLARE_PUBLIC(EntityMimeTypeFilterModel)
    EntityMimeTypeFilterModel *q_ptr;

    QStringList includedMimeTypes;
    QStringList excludedMimeTypes;
    QStringList includedContentMimeTypes;

    QPersistentModelIndex m_rootIndex;

    EntityTreeModel::HeaderGroup m_headerGroup;
};

}

EntityMimeTypeFilterModel::EntityMimeTypeFilterModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d_ptr( new EntityMimeTypeFilterModelPrivate( this ) )
{
}

EntityMimeTypeFilterModel::~EntityMimeTypeFilterModel()
{
  delete d_ptr;
}

void EntityMimeTypeFilterModel::addMimeTypeInclusionFilters(const QStringList &typeList)
{
  Q_D(EntityMimeTypeFilterModel);
  d->includedMimeTypes << typeList;
  invalidateFilter();
}

void EntityMimeTypeFilterModel::addMimeTypeExclusionFilters(const QStringList &typeList)
{
  Q_D(EntityMimeTypeFilterModel);
  d->excludedMimeTypes << typeList;
  invalidateFilter();
}

void EntityMimeTypeFilterModel::addMimeTypeInclusionFilter(const QString &type)
{
  Q_D(EntityMimeTypeFilterModel);
  d->includedMimeTypes << type;
  invalidateFilter();
}

void EntityMimeTypeFilterModel::addMimeTypeExclusionFilter(const QString &type)
{
  Q_D(EntityMimeTypeFilterModel);
  d->excludedMimeTypes << type;
  invalidateFilter();
}

void EntityMimeTypeFilterModel::addContentMimeTypeInclusionFilter( const QString &type )
{
  Q_D(EntityMimeTypeFilterModel);
  d->includedContentMimeTypes << type;
  invalidateFilter();
}

void EntityMimeTypeFilterModel::addContentMimeTypeInclusionFilters( const QStringList &typeList )
{
  Q_D(EntityMimeTypeFilterModel);
  d->includedContentMimeTypes << typeList;
  invalidateFilter();
}

bool EntityMimeTypeFilterModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const
{
  Q_D(const EntityMimeTypeFilterModel);
  // All rows that are not below m_rootIndex are unfiltered.

  bool found = false;

  const bool rootIsValid = d->m_rootIndex.isValid();
  QModelIndex _parent = sourceParent;
  while (true)
  {
    if (_parent == d->m_rootIndex)
    {
      found = true;
      break;
    }
    _parent = _parent.parent();
    if (!_parent.isValid() && rootIsValid)
    {
      break;
    }
  }

  if (!found)
  {
    return true;
  }

  const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);

  const QString rowMimetype = idx.data( EntityTreeModel::MimeTypeRole ).toString();

  if ( d->excludedMimeTypes.contains( rowMimetype ) )
    return false;

  if ( !d->includedMimeTypes.isEmpty() ) {
    if ( d->includedMimeTypes.contains( rowMimetype ) )
      return true;
  }

  if ( !d->includedContentMimeTypes.isEmpty() ) {
    const Collection collection = idx.data( EntityTreeModel::CollectionRole ).value<Collection>();
    if ( collection.isValid() ) {
      const QStringList contentMimeTypes = collection.contentMimeTypes();
      if ( !contentMimeTypes.toSet().intersect( d->includedContentMimeTypes.toSet() ).isEmpty() )
        return true;
      else
        return false;
    } else
      return false;
  }

  return true;
}

QStringList EntityMimeTypeFilterModel::mimeTypeInclusionFilters() const
{
  Q_D(const EntityMimeTypeFilterModel);
  return d->includedMimeTypes;
}

QStringList EntityMimeTypeFilterModel::mimeTypeExclusionFilters() const
{
  Q_D(const EntityMimeTypeFilterModel);
  return d->excludedMimeTypes;
}

QStringList EntityMimeTypeFilterModel::contentMimeTypeInclusionFilters() const
{
  Q_D(const EntityMimeTypeFilterModel);
  return d->includedContentMimeTypes;
}

void EntityMimeTypeFilterModel::clearFilters()
{
  Q_D(EntityMimeTypeFilterModel);
  d->includedMimeTypes.clear();
  d->excludedMimeTypes.clear();
  d->includedContentMimeTypes.clear();
  invalidateFilter();
}

void EntityMimeTypeFilterModel::setHeaderGroup(EntityTreeModel::HeaderGroup headerGroup)
{
  Q_D(EntityMimeTypeFilterModel);
  d->m_headerGroup = headerGroup;
}


QVariant EntityMimeTypeFilterModel::headerData(int section, Qt::Orientation orientation, int role ) const
{
  Q_D(const EntityMimeTypeFilterModel);
  role += (EntityTreeModel::TerminalUserRole * d->m_headerGroup);
  return sourceModel()->headerData(section, orientation, role);
}

bool EntityMimeTypeFilterModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_ASSERT(sourceModel());
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
}

QMimeData* EntityMimeTypeFilterModel::mimeData( const QModelIndexList & indexes ) const
{
  Q_ASSERT(sourceModel());
  QModelIndexList sourceIndexes;
  foreach(const QModelIndex& index, indexes)
    sourceIndexes << mapToSource(index);
  return sourceModel()->mimeData(sourceIndexes);
}

QStringList EntityMimeTypeFilterModel::mimeTypes() const
{
  Q_ASSERT(sourceModel());
  return sourceModel()->mimeTypes();
}

QModelIndexList EntityMimeTypeFilterModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
{
  if (EntityTreeModel::AmazingCompletionRole != role)
    return QSortFilterProxyModel::match(start, role, value, hits, flags);

  // We match everything in the source model because sorting will change what we should show.
  const int allHits = -1;

  QModelIndexList proxyList;
  QMap<int, QModelIndex> proxyMap;
  QModelIndexList sourceList = sourceModel()->match(mapToSource(start), role, value, allHits, flags);
  QModelIndexList::const_iterator it;
  const QModelIndexList::const_iterator begin = sourceList.constBegin();
  const QModelIndexList::const_iterator end = sourceList.constEnd();
  QModelIndex proxyIndex;
  for (it = begin; it != end; ++it)
  {
    proxyIndex = mapFromSource(*it);

    // Any filtered indexes will be invalid when mapped.
    if (!proxyIndex.isValid())
      continue;

    // Inserting in a QMap gives us sorting by key for free.
    proxyMap.insert(proxyIndex.row(), proxyIndex);
  }

  if (hits == -1)
    return proxyMap.values();

  return proxyMap.values().mid(0, hits);
}

int EntityMimeTypeFilterModel::columnCount(const QModelIndex &parent) const
{
  Q_D(const EntityMimeTypeFilterModel);

  QVariant var = sourceModel()->data(parent, EntityTreeModel::ColumnCountRole + (EntityTreeModel::TerminalUserRole * d->m_headerGroup));
  if( !var.isValid() )
    return 0;
  return var.toInt();
}

bool EntityMimeTypeFilterModel::hasChildren(const QModelIndex &parent) const
{
  if ( !parent.isValid() )
    return sourceModel()->hasChildren(parent);

  Q_D(const EntityMimeTypeFilterModel);
  if ( EntityTreeModel::ItemListHeaders == d->m_headerGroup)
    return false;

  if ( EntityTreeModel::CollectionTreeHeaders == d->m_headerGroup )
  {
    QModelIndex childIndex = parent.child( 0, 0 );
    while ( childIndex.isValid() )
    {
      Collection col = childIndex.data( EntityTreeModel::CollectionRole ).value<Collection>();
      if (col.isValid())
        return true;
      childIndex = childIndex.sibling( childIndex.row() + 1, childIndex.column() );
    }
  }
  return false;
}

bool EntityMimeTypeFilterModel::canFetchMore( const QModelIndex &parent ) const
{
  Q_D(const EntityMimeTypeFilterModel);
  if ( EntityTreeModel::CollectionTreeHeaders == d->m_headerGroup )
    return false;
  return QSortFilterProxyModel::canFetchMore(parent);
}

#include "entitymimetypefiltermodel.moc"

