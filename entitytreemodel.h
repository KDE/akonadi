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

#ifndef AKONADI_ENTITYTREEMODEL_H
#define AKONADI_ENTITYTREEMODEL_H

#include "akonadi_export.h"

#include "collectionmodel.h"

// #include <QStringList>

class QStringList;

namespace Akonadi
{

class Collection;
class Item;

class EntityTreeModelPrivate;

/**
 * @short A model for collections and items together.
 *
 * This class provides the interface of QAbstractItemModel for the
 * collection and item tree of the Akonadi storage.
 *
 * Child elements of a collection consist of the child collections
 * followed by the items. This arrangement can be modified using a proxy model.
 *
 * @code
 *
 *   EntityTreeModel *model = new EntityTreeModel( this, QStringList() << FooMimeType << BarMimeType );
 *
 *   QTreeView *view = new QTreeView( this );
 *   view->setModel( model );
 *
 * @endcode
 *
 * Only collections and items matching @p mimeTypes will be shown. This way,
 * retrieving every item in Akonadi is avoided.
 *
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.2
 * @todo Implement dropMimeData. Currently just a stub.
 * @todo Also need to implement a new view to use this.
 * @todo Implement itemMoved.
 */
class AKONADI_EXPORT EntityTreeModel : public CollectionModel
{
    Q_OBJECT

  public:
    /**
     * Describes the roles for items. Roles for collections are defined by the superclass.
     */
    enum Roles {
      ItemRole = Akonadi::CollectionModel::UserRole + 1,              ///< The item.
      ItemIdRole,                                                     ///< The item id
      MimeTypeRole,                                                   ///< The mimetype of the entity
      RemoteIdRole,                                                   ///< The remoteId of the entity
//    EntityAboveRole,                                                ///< The remoteId of the entity above @p index.
      UserRole = Akonadi::CollectionModel::UserRole + 20              ///< Role for user extensions.
    };

    /**
     * Creates a new collection and item model.
     *
     * @param parent The parent object.
     * @param mimeTypes The list of mimetypes to be retrieved in the model.
     */
    explicit EntityTreeModel( const QStringList &mimeTypes, QObject *parent = 0 );

    /**
     * Destroys the collection and item model.
     */
    virtual ~EntityTreeModel();

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex & index ) const;
    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );

    // TODO:
    virtual bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

  protected:
    /**
     * Returns true if the index @p index refers to an item.
     */
    bool isItem( const QModelIndex &index ) const;

    /**
     * Returns true if the index @p index refers to a collection.
     */
    bool isCollection( const QModelIndex &index ) const;

  private:
    Q_DECLARE_PRIVATE( EntityTreeModel )

    Q_PRIVATE_SLOT( d_func(), void listDone( KJob* ) )
    Q_PRIVATE_SLOT( d_func(), void itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) )
    Q_PRIVATE_SLOT( d_func(), void itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void itemAdded( const Akonadi::Item&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d_func(), void itemsAdded( const Akonadi::Item::List& ) )
    Q_PRIVATE_SLOT( d_func(), void itemRemoved( const Akonadi::Item& ) )
    Q_PRIVATE_SLOT( d_func(), void onRowsInserted( const QModelIndex &parent, int start, int end ) )
};

} // namespace

#endif
