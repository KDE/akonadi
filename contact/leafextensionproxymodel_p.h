/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#ifndef LEAFEXTENSIONPROXYMODEL_H
#define LEAFEXTENSIONPROXYMODEL_H

#include <QSortFilterProxyModel>

namespace Akonadi {

class LeafExtensionProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    explicit LeafExtensionProxyModel( QObject *parent = 0 );
    ~LeafExtensionProxyModel();

    QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex parent( const QModelIndex &index ) const;
    int rowCount( const QModelIndex &index ) const;
    int columnCount( const QModelIndex &index ) const;

    QVariant data( const QModelIndex &proxyIndex, int role = Qt::DisplayRole ) const;
    Qt::ItemFlags flags( const QModelIndex &index ) const;
    bool setData( const QModelIndex &index, const QVariant &data, int role = Qt::EditRole );
    bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;
    QModelIndex buddy( const QModelIndex &index ) const;
    void fetchMore( const QModelIndex &index );

    void setSourceModel( QAbstractItemModel *sourceModel );

  protected:
    /**
     * This method is called to retrieve the row count for the given leaf @p index.
     */
    virtual int leafRowCount( const QModelIndex &index ) const = 0;

    /**
     * This method is called to retrieve the column count for the given leaf @p index.
     */
    virtual int leafColumnCount( const QModelIndex &index ) const = 0;

    /**
     * This method is called to retrieve the data of the child of the given leaf @p index
     * at @p row and @p column with the given @p role.
     */
    virtual QVariant leafData( const QModelIndex &index, int row, int column, int role = Qt::DisplayRole ) const = 0;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void sourceRowsInserted( const QModelIndex&, int, int ) )
    Q_PRIVATE_SLOT( d, void sourceRowsRemoved( const QModelIndex&, int, int ) )
    //@endcond
};

}

#endif
