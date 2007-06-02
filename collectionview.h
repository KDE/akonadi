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

#ifndef AKONADI_COLLECTION_VIEW
#define AKONADI_COLLECTION_VIEW

#include "libakonadi_export.h"
#include <QtGui/QTreeView>

class QDragMoveEvent;

namespace Akonadi {

/**
  A view to show a collection tree provided by a CollectionModell.
  It uses an internal QSortFilterProxyModel to provide sorting.
*/
class AKONADI_EXPORT CollectionView : public QTreeView
{
  Q_OBJECT

  public:
    /**
      Create a new collection view.
      @param parent the parent widget.
    */
    explicit CollectionView( QWidget *parent = 0 );

    /**
      Destroys this collection view.
    */
    virtual ~CollectionView();

    /**
      Reimplemented from QAbstractItemView.
    */
    virtual void setModel ( QAbstractItemModel * model );

  protected:
    virtual void dragMoveEvent( QDragMoveEvent *event );
    virtual void dragLeaveEvent( QDragLeaveEvent *event );
    virtual void dropEvent( QDropEvent *event );
    virtual void contextMenuEvent( QContextMenuEvent *event );

  private:
    class Private;
    Private * const d;

    Q_PRIVATE_SLOT( d, void dragExpand() )
    Q_PRIVATE_SLOT( d, void createCollection() )
    Q_PRIVATE_SLOT( d, void createResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void deleteCollection() )
    Q_PRIVATE_SLOT( d, void deleteResult( KJob* ) )
    Q_PRIVATE_SLOT( d, void updateActions( const QModelIndex& ) )
};

}

#endif
