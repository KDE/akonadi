/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include <QtGui/QTreeView>
#include <kdepim_export.h>

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
    CollectionView( QWidget *parent = 0 );

    /**
      Destroys this collection view.
    */
    virtual ~CollectionView();

    /**
      Reimplemented from QAbstractItemView.
    */
    virtual void setModel ( QAbstractItemModel * model );

  public Q_SLOTS:
    /**
      Add a child collection to the given model index.
    */
    void createCollection( const QModelIndex &index );

  protected:
    virtual void dragMoveEvent( QDragMoveEvent *event );
    virtual void dragLeaveEvent( QDragLeaveEvent *event );
    virtual void dropEvent( QDropEvent *event );

    /**
      Translates a QModelIndex from the sort proxy model into a QModelIndex
      from the CollectionModel if necessary.
    */
    QModelIndex sourceIndex( const QModelIndex &index );

  private slots:
    void dragExpand();

  private:
    class Private;
    Private * const d;
};

}

#endif
