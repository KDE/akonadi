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

class KXmlGuiWindow;
class QDragMoveEvent;

namespace Akonadi {

class Collection;

/**
  A view to show a collection tree provided by a CollectionModell.
  It uses an internal QSortFilterProxyModel to provide sorting.

  When a KXmlGuiWindow is passed to the constructor, the XMLGUI
  defined context menu @c akonadi_collectionview_contextmenu is
  used if available.

  @todo The ctor API is not compatible with designer, change that to match ItemView
*/
class AKONADI_EXPORT CollectionView : public QTreeView
{
  Q_OBJECT

  public:
    /**
      Create a new collection view.
      @param xmlGuiWindow The KXmlGuiWindow this is used in. This is needed for the
      XMLGUI based context menu. Passing 0 is ok and will disable the builtin context
      menu.
      @param parent the parent widget.
    */
    explicit CollectionView( KXmlGuiWindow *xmlGuiWindow = 0, QWidget *parent = 0 );

    /**
      Destroys this collection view.
    */
    virtual ~CollectionView();

    /**
      Reimplemented from QAbstractItemView.
    */
    virtual void setModel ( QAbstractItemModel * model );


  Q_SIGNALS:
    /**
     * This signal is emitted whenever the user has clicked
     * a collection in the view.
     */
    void clicked( const Akonadi::Collection &collection );

    /**
     * This signal is emitted whenever the current collection
     * in the view has changed.
     */
    void currentChanged( const Akonadi::Collection &collection );

  protected:
    using QTreeView::currentChanged;
    virtual void dragMoveEvent( QDragMoveEvent *event );
    virtual void dragLeaveEvent( QDragLeaveEvent *event );
    virtual void dropEvent( QDropEvent *event );
    virtual void contextMenuEvent( QContextMenuEvent *event );

  private:
    class Private;
    Private * const d;

    Q_PRIVATE_SLOT( d, void dragExpand() )
    Q_PRIVATE_SLOT( d, void itemClicked( const QModelIndex& ) )
    Q_PRIVATE_SLOT( d, void itemCurrentChanged( const QModelIndex& ) )
};

}

#endif
