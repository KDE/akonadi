/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_ITEMMODEL_H
#define AKONADI_ITEMMODEL_H

#include "akonadi_export.h"
#include <akonadi/item.h>
#include <akonadi/job.h>

#include <QtCore/QAbstractTableModel>

namespace Akonadi {

class Collection;
class ItemFetchScope;
class Job;
class Session;

/**
 * @short A table model for items.
 *
 * A self-updating table model that shows all items of
 * a collection.
 *
 * @code
 *
 * QTableView *view = new QTableView( this );
 *
 * Akonadi::ItemModel *model = new Akonadi::ItemModel();
 * view->setModel( model );
 *
 * model->setCollection( Akonadi::Collection::root() );
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT ItemModel : public QAbstractTableModel
{
  Q_OBJECT

  public:
    /**
     * Describes the types of the columns in the model.
     */
    enum Column {
      Id = 0,     ///< The unique id.
      RemoteId,   ///< The remote identifier.
      MimeType    ///< The item's mime type.
    };

    /**
     * Describes the roles of the model.
     */
    enum Roles {
      IdRole = Qt::UserRole + 1,      ///< The id of the item.
      ItemRole,                       ///< The item object.
      MimeTypeRole,                   ///< The mime type of the item.
      UserRole = Qt::UserRole + 42    ///< Role for user extensions.
    };

    /**
     * Creates a new item model.
     *
     * @param parent The parent object.
     */
    explicit ItemModel( QObject* parent = 0 );

    /**
     * Destroys the item model.
     */
    virtual ~ItemModel();

    virtual int columnCount( const QModelIndex & parent = QModelIndex() ) const;

    virtual QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    virtual int rowCount( const QModelIndex & parent = QModelIndex() ) const;

    virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;

    virtual QMimeData *mimeData( const QModelIndexList &indexes ) const;

    virtual QStringList mimeTypes() const;

    virtual Qt::DropActions supportedDropActions() const;

    /**
     * Sets the item fetch scope.
     *
     * The ItemFetchScope controls how much of an item's data is fetched from the
     * server, e.g. whether to fetch the full item payload or only meta data.
     *
     * @param fetchScope The new scope for item fetch operations.
     *
     * @see fetchScope()
     */
    void setFetchScope( const ItemFetchScope &fetchScope );

    /**
     * Returns the item fetch scope.
     *
     * Since this returns a reference it can be used to conveniently modify the
     * current scope in-place, i.e. by calling a method on the returned reference
     * without storing it in a local variable. See the ItemFetchScope documentation
     * for an example.
     *
     * @return a reference to the current item fetch scope.
     *
     * @see setFetchScope() for replacing the current item fetch scope.
     */
    ItemFetchScope &fetchScope();

    /**
     * Returns the item at the given @p index.
     */
    Item itemForIndex( const QModelIndex &index ) const;

    /**
     * Returns the model index for the given item, with the given column.
     *
     * @param item The item to find.
     * @param column The column for the returned index.
     */
    QModelIndex indexForItem( const Akonadi::Item& item, const int column ) const;

    bool dropMimeData( const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent );

    /**
     * Returns the collection being displayed in the model.
     */
    Collection collection() const;

  public Q_SLOTS:
    /**
     * Sets the collection the model should display. If the collection has
     * changed, the model is reset and a new message listing is requested
     * from the Akonadi storage.
     *
     * @param collection The collection.
     */
    void setCollection( const Akonadi::Collection &collection );

  Q_SIGNALS:
    /**
     * This signal is emitted whenever setCollection is called.
     *
     * @param collection The new collection.
     */
     void collectionChanged( const Akonadi::Collection &collection );

  protected:
    /**
     * Returns the Session object used for all operations by this model.
     */
    Session* session() const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void listingDone( KJob* ) )
    Q_PRIVATE_SLOT( d, void collectionFetchResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void itemChanged( const Akonadi::Item&, const QSet<QByteArray>& ) )
    Q_PRIVATE_SLOT( d, void itemMoved( const Akonadi::Item&, const Akonadi::Collection&, const Akonadi::Collection& ) )
    Q_PRIVATE_SLOT( d, void itemAdded( const Akonadi::Item& ) )
    Q_PRIVATE_SLOT( d, void itemsAdded( const Akonadi::Item::List& ) )
    Q_PRIVATE_SLOT( d, void itemRemoved( const Akonadi::Item& ) )
    //@endcond
};

}

#endif
