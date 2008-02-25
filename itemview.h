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

class KXmlGuiWindow;

namespace Akonadi {

class DataReference;

/**
  A view to show an item list provided by an ItemModel.

  When a KXmlGuiWindow is set, the XMLGUI defined context menu
  @c akonadi_itemview_contextmenu is used if available.

  @todo Convenience ctor including the KXmlGuiWindow for non-designer users?
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

    /**
      Sets the KXmlGuiWindow which this view is used in. This is needed
      if you want to use the built-in context menu.
      @param xmlGuiWindow The KXmlGuiWindow this view is used in.
    */
    void setKXmlGuiWindow( KXmlGuiWindow *xmlGuiWindow );

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

  protected:
    void contextMenuEvent( QContextMenuEvent *event );

  private:
    class Private;
    Private * const d;

    Q_PRIVATE_SLOT( d, void itemActivated( const QModelIndex& ) )
    Q_PRIVATE_SLOT( d, void itemCurrentChanged( const QModelIndex& ) )
};

}

#endif
