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

#include "collectionfiltermodel_p.h"

#include <akonadi/entitytreemodel.h>

CollectionFilterModel::CollectionFilterModel( QObject *parent )
  : QSortFilterProxyModel( parent ), mRights( Akonadi::Collection::ReadOnly )
{
}

void CollectionFilterModel::addContentMimeTypeFilter( const QString &mimeType )
{
  mContentMimeTypes.insert( mimeType );
  invalidateFilter();
}

void CollectionFilterModel::setRightsFilter( Akonadi::Collection::Rights rights )
{
  mRights = rights;
  invalidateFilter();
}

bool CollectionFilterModel::filterAcceptsRow( int row, const QModelIndex &parent ) const
{
  bool accepted = true;

  const QModelIndex index = sourceModel()->index( row, 0, parent );
  const Akonadi::Collection collection = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
  if ( !collection.isValid() )
    return false;

  if ( !mContentMimeTypes.isEmpty() ) {
    QSet<QString> contentMimeTypes = collection.contentMimeTypes().toSet();
    accepted = accepted && !(contentMimeTypes.intersect( mContentMimeTypes ).isEmpty());
  }

  accepted = accepted && (collection.rights() & mRights);

  return accepted;
}

#include "collectionfiltermodel_p.moc"
