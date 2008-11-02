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

#include <QHash>

class QModelIndex;

class KJob;

namespace Akonadi
{
class Collection;
class Monitor;
}

#include "item.h"
#include "collectionmodel_p.h"

#include "entitytreemodel.h"

namespace Akonadi
{

class EntityTreeModelPrivate : public CollectionModelPrivate
{
  public:

    EntityTreeModelPrivate( EntityTreeModel *parent );

    bool mimetypeMatches( const QStringList &mimetypes );

    void listDone( KJob* );
    void itemChanged( const Item&, const QSet<QByteArray>& );
    void itemsAdded( const Item::List &list );

    /*
     * Warning: This slot should never be called directly. It should only be connected to by the itemsRetrieved signal
     * of the ItemFetchJob.
     */
    void itemAdded( const Item &item, const Collection& );

    void itemMoved( const Item&, const Collection& src, const Collection& dst );
    void itemRemoved( const Item& );

    /*
     * Adds items to the model when notified to do so for @p parent.
     */
    void onRowsInserted( const QModelIndex &parent, int start, int end );

    int childEntitiesCount( const QModelIndex & parent ) const;

    QHash< Collection::Id, QList< Item::Id > > m_itemsInCollection;
    QHash< Item::Id, Item > m_items;
    QStringList m_mimeTypeFilter;

    Monitor *itemMonitor;

    QModelIndex indexForItem( Item );

    /*
     * The id of the collection which starts an item fetch job. This is part of a hack with QObject::sender
     * in itemsAdded to correctly insert items into the model.
     */
    static QByteArray ItemFetchCollectionId() {
      return "ItemFetchCollectionId";
    }

    Q_DECLARE_PUBLIC( EntityTreeModel )
};

}

#endif

