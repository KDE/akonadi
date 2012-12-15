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

#ifndef EMAILADDRESSSELECTIONPROXYMODEL_H
#define EMAILADDRESSSELECTIONPROXYMODEL_H

#include "leafextensionproxymodel_p.h"

#include "contactstreemodel.h"

namespace Akonadi {

class EmailAddressSelectionProxyModel : public Akonadi::LeafExtensionProxyModel
{
  Q_OBJECT

  public:
    enum Role {
      NameRole = ContactsTreeModel::DateRole + 1,
      EmailAddressRole
    };

    explicit EmailAddressSelectionProxyModel( QObject *parent = 0 );
    ~EmailAddressSelectionProxyModel();

    QVariant data( const QModelIndex &proxyIndex, int role = Qt::DisplayRole ) const;

  protected:
    /**
     * This method is called to retrieve the row count for the given leaf @p index.
     */
    int leafRowCount( const QModelIndex &index ) const;

    /**
     * This methid is called to retrieve the column count for the given leaf @p index.
     */
    int leafColumnCount( const QModelIndex &index ) const;

    /**
     * This methid is called to retrieve the data of the child of the given leaf @p index
     * at @p row and @p column with the given @p role.
     */
    QVariant leafData( const QModelIndex &index, int row, int column, int role = Qt::DisplayRole ) const;

  private:
};

}

#endif
