/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>

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

#include "collectionfilterproxymodel.h"

#include "collectionmodel.h"

#include <QString>
#include <QStringList>

using namespace Akonadi;

class CollectionFilterProxyModel::Private
{
  public:
    Private( CollectionFilterProxyModel *parent )
      : mParent( parent ), sourceModel( 0 )
    {
    }

    bool collectionAccepted( const QModelIndex &index );

    CollectionFilterProxyModel *mParent;
    QStringList mimeTypes;
    CollectionModel *sourceModel;
};

bool CollectionFilterProxyModel::Private::collectionAccepted( const QModelIndex &index )
{
  // Retrieve supported mimetypes
  QStringList collectionMimeTypes = mParent->sourceModel()->data( index, CollectionModel::CollectionContentTypesRole ).toStringList();

  // If this collection directly contains one valid mimetype, it is accepted
  foreach ( QString type, mimeTypes ) {
    if ( collectionMimeTypes.contains( type ) )
      return true;
  }
  // If this collection has a child which contains valid mimetypes, it is accepted
  QModelIndex childIndex = index.child( 0, 0 );
  while ( childIndex.isValid() ) {
    if ( collectionAccepted( childIndex ) )
      return true;

    childIndex = childIndex.sibling( childIndex.row() + 1, 0 );
  }

  // Or else, no reason to keep this collection.
  return false;
}


CollectionFilterProxyModel::CollectionFilterProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ),
    d( new Private( this ) )
{
}

CollectionFilterProxyModel::~CollectionFilterProxyModel()
{
  delete d;
}

void CollectionFilterProxyModel::addMimeTypes(const QStringList &typeList)
{
  d->mimeTypes << typeList;
}

void CollectionFilterProxyModel::addMimeType(const QString &type)
{
  d->mimeTypes << type;
}

bool CollectionFilterProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const
{
  return d->collectionAccepted( sourceModel()->index( sourceRow, 0, sourceParent ) );
}

QStringList CollectionFilterProxyModel::mimeTypes() const
{
  return d->mimeTypes;
}

#include "collectionfilterproxymodel.moc"
