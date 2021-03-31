/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Stephen Kelly <steveire@gmail.com>
    SPDX-FileCopyrightText: 2012-2021 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"

#include <QTreeView>

class KXMLGUIClient;
class QDragMoveEvent;

namespace Akonadi
{
class Collection;
class Item;

/**
 * @short A view to show an item/collection tree provided by an EntityTreeModel.
 *
 * When a KXmlGuiWindow is passed to the constructor, the XMLGUI
 * defined context menu @c akonadi_collectionview_contextmenu or
 * @c akonadi_itemview_contextmenu is used if available.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * class MyWindow : public KXmlGuiWindow
 * {
 *   public:
 *    MyWindow()
 *      : KXmlGuiWindow()
 *    {
 *      EntityTreeView *view = new EntityTreeView( this, this );
 *      setCentralWidget( view );
 *
 *      EntityTreeModel *model = new EntityTreeModel( ... );
 *      view->setModel( model );
 *    }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 * @author Stephen Kelly <steveire@gmail.com>
 * @since 4.4
 */
class AKONADIWIDGETS_EXPORT EntityTreeView : public QTreeView
{
    Q_OBJECT

public:
    /**
     * Creates a new entity tree view.
     *
     * @param parent The parent widget.
     */
    explicit EntityTreeView(QWidget *parent = nullptr);

    /**
     * Creates a new entity tree view.
     *
     * @param xmlGuiClient The KXMLGUIClient the view is used in.
     *                     This is needed for the XMLGUI based context menu.
     *                     Passing 0 is ok and will disable the builtin context menu.
     * @param parent The parent widget.
     */
    explicit EntityTreeView(KXMLGUIClient *xmlGuiClient, QWidget *parent = nullptr);

    /**
     * Destroys the entity tree view.
     */
    ~EntityTreeView() override;

    /**
     * Sets the XML GUI client which the view is used in.
     *
     * This is needed if you want to use the built-in context menu.
     *
     * @param xmlGuiClient The KXMLGUIClient the view is used in.
     */
    void setXmlGuiClient(KXMLGUIClient *xmlGuiClient);

    /**
     * Return the XML GUI client which the view is used in.
     * @since 4.12
     */
    KXMLGUIClient *xmlGuiClient() const;

    /**
     * @reimp
     * @param model the model to set
     */
    void setModel(QAbstractItemModel *model) override;

    /**
     * Sets whether the drop action menu is @p enabled and will
     * be shown on drop operation.
     * @param enabled enables drop action menu if set as @c true
     * @since 4.5
     */
    void setDropActionMenuEnabled(bool enabled);

    /**
     * Returns whether the drop action menu is enabled and will
     * be shown on drop operation.
     *
     * @since 4.5
     */
    Q_REQUIRED_RESULT bool isDropActionMenuEnabled() const;

    /**
     * Return true if we use an manual sorting
     * Necessary to fix dnd menu
     * We must show just move when we move item between two items
     * When automatic no show dnd menu between two items.
     * @since 4.8.1
     */
    Q_REQUIRED_RESULT bool isManualSortingActive() const;

    /**
     * Set true if we automatic sorting
     * @param active enables automatic sorting if set as @c true
     * @since 4.8.1
     */
    void setManualSortingActive(bool active);

    /**
     * Set the name of the default popup menu (retrieved from the
     * application's XMLGUI file).
     *
     * This menu is used as a fallback if the context of the menu request
     * is neither an item nor a collection, e.g. the click is on an empty
     * area inside the view.  If the click is over an entry in the view,
     * the menu which is applicable to the clicked entry (either an Item
     * or a Collection) is used.
     *
     * @param name The name of the popup menu
     *
     * @since 4.9
     * @note For backwards compatibility, the default is the standard
     * collection popup menu, "akonadi_collectionview_contextmenu".
     * @see KXMLGUIClient, KXMLGUIFactory::container()
     */
    void setDefaultPopupMenu(const QString &name);

Q_SIGNALS:
    /**
     * This signal is emitted whenever the user has clicked
     * a collection in the view.
     *
     * @param collection The clicked collection.
     */
    void clicked(const Akonadi::Collection &collection);

    /**
     * This signal is emitted whenever the user has clicked
     * an item in the view.
     *
     * @param item The clicked item.
     */
    void clicked(const Akonadi::Item &item);

    /**
     * This signal is emitted whenever the user has double clicked
     * a collection in the view.
     *
     * @param collection The double clicked collection.
     */
    void doubleClicked(const Akonadi::Collection &collection);

    /**
     * This signal is emitted whenever the user has double clicked
     * an item in the view.
     *
     * @param item The double clicked item.
     */
    void doubleClicked(const Akonadi::Item &item);

    /**
     * This signal is emitted whenever the current collection
     * in the view has changed.
     *
     * @param collection The new current collection.
     */
    void currentChanged(const Akonadi::Collection &collection);

    /**
     * This signal is emitted whenever the current item
     * in the view has changed.
     *
     * @param item The new current item.
     */
    void currentChanged(const Akonadi::Item &item);

protected:
    using QTreeView::currentChanged;
#ifndef QT_NO_DRAGANDDROP
    void startDrag(Qt::DropActions supportedActions) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
#endif
    void timerEvent(QTimerEvent *event) override;
#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif

private:
    /// @cond PRIVATE
    class Private;
    Private *const d;
    /// @endcond
};

}

