/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTION_VIEW
#define AKONADI_COLLECTION_VIEW

#include "akonadiwidgets_export.h"
#include <QTreeView>

class KXMLGUIClient;
class KXmlGuiWindow;
class QDragMoveEvent;

namespace Akonadi
{

class Collection;

/**
 * @short A view to show a collection tree provided by a EntityTreeModel.
 *
 * When a KXmlGuiWindow is passed to the constructor, the XMLGUI
 * defined context menu @c akonadi_collectionview_contextmenu is
 * used if available.
 *
 * Example:
 *
 * @code
 *
 * class MyWindow : public KXmlGuiWindow
 * {
 * public:
 *     MyWindow()
 *       : KXmlGuiWindow()
 *     {
 *         Akonadi::CollectionView *view = new Akonadi::CollectionView(this, this);
 *         setCentralWidget(view);
 *
 *         Akonadi::Monitor *monitor = new Akonadi::Monitor(this);
 *         Akonadi::EntityTreeModel *model = new Akonadi::EntityTreeModel(monitor, this);
 *         view->setModel(model);
 *     }
 * }
 *
 * @endcode
 *
 * @deprecated Use EntityTreeView or EntityListView on top of EntityTreeModel instead.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_DEPRECATED_EXPORT CollectionView : public QTreeView
{
    Q_OBJECT

public:
    /**
     * Creates a new collection view.
     *
     * @param parent The parent widget.
     */
    explicit CollectionView(QWidget *parent = nullptr);

    /**
     * Creates a new collection view.
     *
     * @param xmlGuiClient The KXmlGuiClient the view is used in.
     *                     This is needed for the XMLGUI based context menu.
     *                     Passing 0 is ok and will disable the builtin context menu.
     * @param parent The parent widget.
     */
    explicit CollectionView(KXMLGUIClient *xmlGuiClient, QWidget *parent = nullptr);

    /**
     * Destroys the collection view.
     */
    ~CollectionView() override;

    /**
     * Sets the KXMLGUIClient which the view is used in.
     * This is needed if you want to use the built-in context menu.
     *
     * @param xmlGuiClient The KXMLGUIClient the view is used in.
     * @since 4.3
     */
    void setXmlGuiClient(KXMLGUIClient *xmlGuiClient);

    void setModel(QAbstractItemModel *model) override;

Q_SIGNALS:
    /**
     * This signal is emitted whenever the user has clicked
     * a collection in the view.
     *
     * @param collection The clicked collection.
     */
    void clicked(const Akonadi::Collection &collection);

    /**
     * This signal is emitted whenever the current collection
     * in the view has changed.
     *
     * @param collection The new current collection.
     */
    void currentChanged(const Akonadi::Collection &collection);

protected:
    using QTreeView::currentChanged;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void itemClicked(const QModelIndex &))
    Q_PRIVATE_SLOT(d, void itemCurrentChanged(const QModelIndex &))
    //@endcond
};

}

#endif
