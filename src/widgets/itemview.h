/*
    SPDX-FileCopyrightText: 2007 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEM_VIEW
#define AKONADI_ITEM_VIEW

#include "akonadiwidgets_export.h"
#include <QTreeView>

class KXmlGuiWindow;
class KXMLGUIClient;
namespace Akonadi
{
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
 * @deprecated Use EntityTreeView or EntityListView on top of EntityTreeModel instead.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADIWIDGETS_DEPRECATED_EXPORT ItemView : public QTreeView
{
    Q_OBJECT

public:
    /**
     * Creates a new item view.
     *
     * @param parent The parent widget.
     */
    explicit ItemView(QWidget *parent = nullptr);

    /**
     * Creates a new item view.
     *
     * @param xmlGuiClient The KXMLGUIClient this is used in.
     *                     This is needed for the XMLGUI based context menu.
     *                     Passing 0 is ok and will disable the builtin context menu.
     * @param parent The parent widget.
     * @since 4.3
     */
    explicit ItemView(KXMLGUIClient *xmlGuiClient, QWidget *parent = nullptr);

    /**
     * Destroys the item view.
     */
    ~ItemView() override;

    /**
     * Sets the KXMLGUIFactory which this view is used in.
     * This is needed if you want to use the built-in context menu.
     *
     * @param xmlGuiClient The KXMLGUIClient this view is used in.
     */
    void setXmlGuiClient(KXMLGUIClient *xmlGuiClient);

    void setModel(QAbstractItemModel *model) override;

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
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    /// @cond PRIVATE
    class Private;
    Private *const d;
    /// @endcond
};

}

#endif
