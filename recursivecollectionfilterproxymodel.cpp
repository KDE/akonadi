/*
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

#include "recursivecollectionfilterproxymodel.h"

#include "entitytreemodel.h"
#include "entityhiddenattribute.h"

#include <kdebug.h>

using namespace Akonadi;

namespace Akonadi
{

class RecursiveCollectionFilterProxyModelPrivate
{
  Q_DECLARE_PUBLIC(RecursiveCollectionFilterProxyModel)
  RecursiveCollectionFilterProxyModel *q_ptr;
public:
  RecursiveCollectionFilterProxyModelPrivate(RecursiveCollectionFilterProxyModel *model)
      : q_ptr(model)
  {

  }

  QSet<QString> includedMimeTypes;
};

}

RecursiveCollectionFilterProxyModel::RecursiveCollectionFilterProxyModel( QObject* parent )
    : KRecursiveFilterProxyModel(parent), d_ptr(new RecursiveCollectionFilterProxyModelPrivate( this ) )
{

}

RecursiveCollectionFilterProxyModel::~RecursiveCollectionFilterProxyModel()
{
  delete d_ptr;
}

bool RecursiveCollectionFilterProxyModel::acceptRow(int sourceRow, const QModelIndex& sourceParent) const
{
  Q_D(const RecursiveCollectionFilterProxyModel);

  QModelIndex rowIndex = sourceModel()->index(sourceRow, 0, sourceParent);
  Akonadi::Collection col = rowIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
  if (!col.isValid())
    return false;

  if (d->includedMimeTypes.isEmpty())
    return true;

  QSet<QString> contentMimeTypes = col.contentMimeTypes().toSet();

  if ( contentMimeTypes.intersect(d->includedMimeTypes).isEmpty())
    return false;
  return true;
}

void RecursiveCollectionFilterProxyModel::addContentMimeTypeInclusionFilter(const QString& mimeType)
{
  Q_D(RecursiveCollectionFilterProxyModel);
  layoutAboutToBeChanged();
  d->includedMimeTypes << mimeType;
  invalidateFilter();
  layoutChanged();
}

void RecursiveCollectionFilterProxyModel::addContentMimeTypeInclusionFilters(const QStringList& mimeTypes)
{
  Q_D(RecursiveCollectionFilterProxyModel);
  layoutAboutToBeChanged();
  d->includedMimeTypes.unite(mimeTypes.toSet());
  invalidateFilter();
  layoutChanged();
}

void RecursiveCollectionFilterProxyModel::clearFilters()
{
  Q_D(RecursiveCollectionFilterProxyModel);
  layoutAboutToBeChanged();
  d->includedMimeTypes.clear();
  invalidateFilter();
  layoutChanged();
}

void RecursiveCollectionFilterProxyModel::setContentMimeTypeInclusionFilters(const QStringList& mimeTypes)
{
  Q_D(RecursiveCollectionFilterProxyModel);
  layoutAboutToBeChanged();
  d->includedMimeTypes = mimeTypes.toSet();
  invalidateFilter();
  layoutChanged();
}

QStringList RecursiveCollectionFilterProxyModel::contentMimeTypeInclusionFilters() const
{
  Q_D(const RecursiveCollectionFilterProxyModel);
  return d->includedMimeTypes.toList();
}
