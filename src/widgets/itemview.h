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

#include "akonadiwidgets_export.h"
#include <QTreeView>

class KXmlGuiWindow;
class KXMLGUIClient;
namespace Akonadi {

class Item;

/**
 * @short A view to show an item list provided by an ItemModel.
 *
 * When a KXmlGuiWindow is set, the XMLGUI defined context menu
 * @c akonadi_itemview_contextmenu is used if available.
 *
 * Example:
 *
 * @code
 *
 * class MyWindow : public KXmlGuiWindow
 * {
 *   public:
 *    MyWindow()
 *      : KXmlGuiWindow()
 *    {
 *      Akonadi::ItemView *view = new Akonadi::ItemView( this, this );
 *      setCentralWidget( view );
 *
 *      Akonadi::ItemModel *model = new Akonadi::ItemModel( this );
 *      view->setModel( model );
 *    }
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADIWIDGETS_EXPORT ItemView : public QTreeView
{
    Q_OBJECT

public:
    /**
     * Creates a new item view.
     *
     * @param parent The parent widget.
     */
    explicit ItemView(QWidget *parent = Q_NULLPTR);

    /**
     * Creates a new item view.
     *
     * @param xmlGuiClient The KXMLGUIClient this is used in.
     *                     This is needed for the XMLGUI based context menu.
     *                     Passing 0 is ok and will disable the builtin context menu.
     * @param parent The parent widget.
     * @since 4.3
     */
    explicit ItemView(KXMLGUIClient *xmlGuiClient, QWidget *parent = Q_NULLPTR);

    /**
     * Destroys the item view.
     */
    virtual ~ItemView();

    /**
     * Sets the KXMLGUIFactory which this view is used in.
     * This is needed if you want to use the built-in context menu.
     *
     * @param xmlGuiClient The KXMLGUIClient this view is used in.
     */
    void setXmlGuiClient(KXMLGUIClient *xmlGuiClient);

    virtual void setModel(QAbstractItemModel *model);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the user has activated
     * an item in the view.
     *
     * @param item The activated item.
     */
    void activated(const Akonadi::Item &item);

    /**
     * This signal is emitted whenever the current item
     * in the view has changed.
     *
     * @param item The current item.
     */
    void currentChanged(const Akonadi::Item &item);

    /**
     * This signal is emitted whenever the user clicked on an item
     * in the view.
     *
     * @param item The item the user clicked on.
     * @since 4.3
     */
    void clicked(const Akonadi::Item &item);

    /**
     * This signal is emitted whenever the user double clicked on an item
     * in the view.
     *
     * @param item The item the user double clicked on.
     * @since 4.3
     */
    void doubleClicked(const Akonadi::Item &item);

protected:
    using QTreeView::currentChanged;
    void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void itemActivated(const QModelIndex &))
    Q_PRIVATE_SLOT(d, void itemCurrentChanged(const QModelIndex &))
    Q_PRIVATE_SLOT(d, void itemClicked(const QModelIndex &))
    Q_PRIVATE_SLOT(d, void itemDoubleClicked(const QModelIndex &))
    //@endcond
};

}

#endif
