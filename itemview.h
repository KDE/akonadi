/*
    Copyright (c) 2007 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ITEM_VIEW
#define AKONADI_ITEM_VIEW

#include "libakonadi_export.h"
#include <QtGui/QTreeView>

namespace Akonadi {

class DataReference;

/**
  A view to show an item list provided by an ItemModel.
*/
class AKONADI_EXPORT ItemView : public QTreeView
{
  Q_OBJECT

  public:
    /**
      Create a new item view.
      @param parent the parent widget.
    */
    explicit ItemView( QWidget *parent = 0 );

    /**
      Destroys this item view.
    */
    virtual ~ItemView();

    /**
      Reimplemented from QAbstractItemView.
    */
    virtual void setModel( QAbstractItemModel * model );


  Q_SIGNALS:
    /**
     * This signal is emitted whenever the user has activated
     * an item in the view.
     */
    void activated( const Akonadi::DataReference &item );

    /**
     * This signal is emitted whenever the current item
     * in the view has changed.
     */
    void currentChanged( const Akonadi::DataReference &item );

  private:
    class Private;
    Private * const d;

    Q_PRIVATE_SLOT( d, void itemActivated( const QModelIndex& ) )
    Q_PRIVATE_SLOT( d, void itemCurrentChanged( const QModelIndex& ) )
};

}

#endif
