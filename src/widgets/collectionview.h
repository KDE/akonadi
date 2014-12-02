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

#include "akonadiwidgets_export.h"
#include <QTreeView>

class KXMLGUIClient;
class KXmlGuiWindow;
class QDragMoveEvent;

namespace Akonadi {

class Collection;

/**
 * @short A view to show a collection tree provided by a CollectionModel.
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
 *   public:
 *    MyWindow()
 *      : KXmlGuiWindow()
 *    {
 *      Akonadi::CollectionView *view = new Akonadi::CollectionView( this, this );
 *      setCentralWidget( view );
 *
 *      Akonadi::CollectionModel *model = new Akonadi::CollectionModel( this );
 *      view->setModel( model );
 *    }
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT CollectionView : public QTreeView
{
    Q_OBJECT

public:
    /**
     * Creates a new collection view.
     *
     * @param parent The parent widget.
     */
    explicit CollectionView(QWidget *parent = 0);

    /**
     * Creates a new collection view.
     *
     * @param xmlGuiWindow The KXmlGuiWindow the view is used in.
     *                     This is needed for the XMLGUI based context menu.
     *                     Passing 0 is ok and will disable the builtin context menu.
     * @param parent The parent widget.
     */
    explicit AKONADIWIDGETS_DEPRECATED CollectionView(KXmlGuiWindow *xmlGuiWindow, QWidget *parent = 0);

    /**
     * Creates a new collection view.
     *
     * @param xmlGuiClient The KXmlGuiClient the view is used in.
     *                     This is needed for the XMLGUI based context menu.
     *                     Passing 0 is ok and will disable the builtin context menu.
     * @param parent The parent widget.
     */
    explicit CollectionView(KXMLGUIClient *xmlGuiClient, QWidget *parent = 0);

    /**
     * Destroys the collection view.
     */
    virtual ~CollectionView();

    /**
     * Sets the KXmlGuiWindow which the view is used in.
     * This is needed if you want to use the built-in context menu.
     *
     * @param xmlGuiWindow The KXmlGuiWindow the view is used in.
     */
    AKONADIWIDGETS_DEPRECATED void setXmlGuiWindow(KXmlGuiWindow *xmlGuiWindow);

    /**
     * Sets the KXMLGUIClient which the view is used in.
     * This is needed if you want to use the built-in context menu.
     *
     * @param xmlGuiClient The KXMLGUIClient the view is used in.
     * @since 4.3
     */
    void setXmlGuiClient(KXMLGUIClient *xmlGuiClient);

    virtual void setModel(QAbstractItemModel *model);

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
    virtual void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    virtual void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    virtual void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    virtual void contextMenuEvent(QContextMenuEvent *event) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void dragExpand())
    Q_PRIVATE_SLOT(d, void itemClicked(const QModelIndex &))
    Q_PRIVATE_SLOT(d, void itemCurrentChanged(const QModelIndex &))
    //@endcond
};

}

#endif
