/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_DRAGDROPMANAGER_P_H
#define AKONADI_DRAGDROPMANAGER_P_H

#include <QAbstractItemView>
#include "collection.h"

namespace Akonadi {

class DragDropManager
{
public:
    explicit DragDropManager(QAbstractItemView *view);

    /**
     * @returns True if the drop described in @p event is allowed.
     */
    bool dropAllowed(QDragMoveEvent *event) const;

    /**
     * Process an attempted drop event.
     *
     * Attempts to show a popup menu with possible actions for @p event.
     *
     * @returns True if the event should be further processed, and false otherwise.
     */
    bool processDropEvent(QDropEvent *event, bool &menuCanceled, bool dropOnItem = true);

    /**
     * Starts a drag if possible and sets the appropriate supported actions to allow moves.
     *
     * Also sets the pixmap for hte drag to something appropriately small, overriding the Qt
     * behaviour of creating a painting of all selected rows when dragging.
     */
    void startDrag(Qt::DropActions supportedActions);

    /**
     * Sets whether to @p show the drop action menu on drop operation.
     */
    void setShowDropActionMenu(bool show);

    /**
     * Returns whether the drop action menu is shown on drop operation.
     */
    bool showDropActionMenu() const;

    bool isManualSortingActive() const;

    /**
     * Set true if we automatic sorting
     */
    void setManualSortingActive(bool active);

private:
    Collection currentDropTarget(QDropEvent *event) const;

    bool hasAncestor(const QModelIndex &index, Collection::Id parentId) const;
    bool mShowDropActionMenu;
    bool mIsManualSortingActive;
    QAbstractItemView *m_view;
};

}

#endif
