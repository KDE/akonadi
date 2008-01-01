/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_FLATCOLLECTIONPROXYMODEL_H
#define AKONADI_FLATCOLLECTIONPROXYMODEL_H

#include <QtGui/QAbstractProxyModel>

namespace Akonadi {

/**
  Converts the hierarchical collection model into a flat one.

  @internal
  @todo Make this fast. And maybe even generic.
*/
class FlatCollectionProxyModel : public QAbstractProxyModel
{
  Q_OBJECT
  public:
    /**
      Create a new flat collection proxy model.
      @param parent The parent object.
    */
    FlatCollectionProxyModel( QObject *parent = 0 );

    /**
      Destructor.
    */
    ~FlatCollectionProxyModel();

    void setSourceModel ( QAbstractItemModel * sourceModel );

    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QModelIndex index( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent( const QModelIndex & index ) const;

    QModelIndex mapFromSource ( const QModelIndex & sourceIndex ) const;
    QModelIndex mapToSource ( const QModelIndex & proxyIndex ) const;

  private:
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void sourceDataChanged( const QModelIndex &, const QModelIndex & ) )
};

}

#endif
