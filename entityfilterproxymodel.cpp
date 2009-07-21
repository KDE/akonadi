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

#include "entityfilterproxymodel.h"

#include "entitytreemodel.h"

#include <kdebug.h>
#include <kmimetype.h>

#include <QtCore/QString>
#include <QtCore/QStringList>

using namespace Akonadi;

namespace Akonadi {
/**
 * @internal
 */
class EntityFilterProxyModelPrivate
{
  public:
    EntityFilterProxyModelPrivate( EntityFilterProxyModel *parent )
      : q_ptr( parent ),
        m_headerSet(0)
    {
    }

    QStringList includedMimeTypes;
    QStringList excludedMimeTypes;

    QPersistentModelIndex m_rootIndex;

    int m_headerSet;

    Q_DECLARE_PUBLIC(EntityFilterProxyModel)
    EntityFilterProxyModel *q_ptr;
};

}

EntityFilterProxyModel::EntityFilterProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d_ptr( new EntityFilterProxyModelPrivate( this ) )
{
  // TODO: Override setSourceModel and do this there?
  setSupportedDragActions( Qt::CopyAction | Qt::MoveAction );
}

EntityFilterProxyModel::~EntityFilterProxyModel()
{
  delete d_ptr;
}

void EntityFilterProxyModel::addMimeTypeInclusionFilters(const QStringList &typeList)
{
  Q_D(EntityFilterProxyModel);
  d->includedMimeTypes << typeList;
  invalidateFilter();
}

void EntityFilterProxyModel::addMimeTypeExclusionFilters(const QStringList &typeList)
{
  Q_D(EntityFilterProxyModel);
  d->excludedMimeTypes << typeList;
  invalidateFilter();
}

void EntityFilterProxyModel::addMimeTypeInclusionFilter(const QString &type)
{
  Q_D(EntityFilterProxyModel);
  d->includedMimeTypes << type;
  invalidateFilter();
}

void EntityFilterProxyModel::addMimeTypeExclusionFilter(const QString &type)
{
  Q_D(EntityFilterProxyModel);
  d->excludedMimeTypes << type;
  invalidateFilter();
}

bool EntityFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const
{
  Q_D(const EntityFilterProxyModel);
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
  if ( d->includedMimeTypes.isEmpty() || d->includedMimeTypes.contains( rowMimetype ) )
    return true;

  return false;
}

QStringList EntityFilterProxyModel::mimeTypeInclusionFilters() const
{
  Q_D(const EntityFilterProxyModel);
  return d->includedMimeTypes;
}

QStringList EntityFilterProxyModel::mimeTypeExclusionFilters() const
{
  Q_D(const EntityFilterProxyModel);
  return d->excludedMimeTypes;
}

void EntityFilterProxyModel::clearFilters()
{
  Q_D(EntityFilterProxyModel);
  d->includedMimeTypes.clear();
  d->excludedMimeTypes.clear();
  invalidateFilter();
}

void EntityFilterProxyModel::setRootIndex(const QModelIndex &srcIndex)
{
  Q_D(EntityFilterProxyModel);
  d->m_rootIndex = srcIndex;
  reset();
}

void EntityFilterProxyModel::setHeaderSet(int set)
{
  Q_D(EntityFilterProxyModel);
  d->m_headerSet = set;
}


QVariant EntityFilterProxyModel::headerData(int section, Qt::Orientation orientation, int role ) const
{
  Q_D(const EntityFilterProxyModel);
  role += (EntityTreeModel::TerminalUserRole * d->m_headerSet);
  return sourceModel()->headerData(section, orientation, role);
}

bool EntityFilterProxyModel::dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent )
{
  Q_ASSERT(sourceModel());
  const QModelIndex sourceParent = mapToSource(parent);
  return sourceModel()->dropMimeData(data, action, row, column, sourceParent);
}

QMimeData* EntityFilterProxyModel::mimeData( const QModelIndexList & indexes ) const
{
  Q_ASSERT(sourceModel());
  QModelIndexList sourceIndexes;
  foreach(const QModelIndex& index, indexes)
    sourceIndexes << mapToSource(index);
  return sourceModel()->mimeData(sourceIndexes);
}

QStringList EntityFilterProxyModel::mimeTypes() const
{
  Q_ASSERT(sourceModel());
  return sourceModel()->mimeTypes();
}

QModelIndexList EntityFilterProxyModel::match(const QModelIndex& start, int role, const QVariant& value, int hits, Qt::MatchFlags flags) const
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

#include "entityfilterproxymodel.moc"

