/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2008 Stephen Kelly <steveire@gmail.com>
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"

#include <QListView>

#include <memory>

class KXMLGUIClient;
class QDragMoveEvent;

namespace Akonadi
{
class Collection;
class Item;
class EntityListViewPrivate;

/*!
 * \brief A view to show an item/collection list provided by an EntityTreeModel.
 *
 * When a KXmlGuiWindow is passed to the constructor, the XMLGUI
 * defined context menu \\ akonadi_collectionview_contextmenu or
 * \\ akonadi_itemview_contextmenu is used if available.
 *
 * Example:
 *
 * \code
 *
 * using namespace Akonadi;
 *
 * class MyWindow : public KXmlGuiWindow
 * {
 *   public:
 *    MyWindow()
 *      : KXmlGuiWindow()
 *    {
 *      EntityListView *view = new EntityListView( this, this );
 *      setCentralWidget( view );
 *
 *      EntityTreeModel *model = new EntityTreeModel( ... );
 *
 *      KDescendantsProxyModel *flatModel = new KDescendantsProxyModel( this );
 *      flatModel->setSourceModel( model );
 *
 *      view->setModel( flatModel );
 *    }
 * }
 *
 * \endcode
 *
 * \author Volker Krause <vkrause@kde.org>
 * \author Stephen Kelly <steveire@gmail.com>
 * \since 4.4
 *
 * \class Akonadi::EntityListView
 * \inheaderfile Akonadi/EntityListView
 * \inmodule AkonadiWidgets
 */
class AKONADIWIDGETS_EXPORT EntityListView : public QListView
{
    Q_OBJECT

public:
    /*!
     * Creates a new favorite collections view.
     *
     * \a parent The parent widget.
     */
    explicit EntityListView(QWidget *parent = nullptr);

    /*!
     * Creates a new favorite collections view.
     *
     * \a xmlGuiClient The KXMLGUIClient the view is used in.
     *                     This is needed for the XMLGUI based context menu.
     *                     Passing 0 is ok and will disable the builtin context menu.
     * \a parent The parent widget.
     */
    explicit EntityListView(KXMLGUIClient *xmlGuiClient, QWidget *parent = nullptr);

    /*!
     * Destroys the favorite collections view.
     */
    ~EntityListView() override;

    /*!
     * Sets the XML GUI client which the view is used in.
     *
     * This is needed if you want to use the built-in context menu.
     *
     * \a xmlGuiClient The KXMLGUIClient the view is used in.
     */
    void setXmlGuiClient(KXMLGUIClient *xmlGuiClient);

    /*!
     * Return the XML GUI client which the view is used in.
     * \since 4.12
     */
    KXMLGUIClient *xmlGuiClient() const;

    /*!
     * \reimp
     * \a model the model to set
     */
    void setModel(QAbstractItemModel *model) override;

    /*!
     * Sets whether the drop action \anu is \a enabled and will
     * \a shown on drop operation.
     * \a enabled enables drop action menu if set as \\ true
     * \since 4.7
     */
    void setDropActionMenuEnabled(bool enabled);

    /*!
     * Returns whether the drop action menu is enabled and will
     * be shown on drop operation.
     *
     * \since 4.7
     */
    [[nodiscard]] bool isDropActionMenuEnabled() const;

Q_SIGNALS:
    /*!
     * This signal is emitted whenever the user has clicked
     * a collection in the view.
     *
     * \a collection The clicked collection.
     */
    void clicked(const Akonadi::Collection &collection);

    /*!
     * This signal is emitted whenever the user has clicked
     * an item in the view.
     *
     * \a item The clicked item.
     */
    void clicked(const Akonadi::Item &item);

    /*!
     * This signal is emitted whenever the user has double clicked
     * a collection in the view.
     *
     * \a collection The double clicked collection.
     */
    void doubleClicked(const Akonadi::Collection &collection);

    /*!
     * This signal is emitted whenever the user has double clicked
     * an item in the view.
     *
     * \a item The double clicked item.
     */
    void doubleClicked(const Akonadi::Item &item);

    /*!
     * This signal is emitted whenever the current collection
     * in the view has changed.
     *
     * \a collection The new current collection.
     */
    void currentChanged(const Akonadi::Collection &collection);

    /*!
     * This signal is emitted whenever the current item
     * in the view has changed.
     *
     * \a item The new current item.
     */
    void currentChanged(const Akonadi::Item &item);

protected:
    using QListView::currentChanged;
#ifndef QT_NO_DRAGANDDROP
    void startDrag(Qt::DropActions supportedActions) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
#endif

#ifndef QT_NO_CONTEXTMENU
    void contextMenuEvent(QContextMenuEvent *event) override;
#endif

private:
    std::unique_ptr<EntityListViewPrivate> const d;
};

}
