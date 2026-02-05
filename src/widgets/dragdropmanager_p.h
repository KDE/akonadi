/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "collection.h"
#include <QAbstractItemView>

namespace Akonadi
{
class DragDropManager
{
public:
    explicit DragDropManager(QAbstractItemView *view);

    /**
     * \returns True if the drop described in \p event is allowed.
     */
    bool dropAllowed(QDragMoveEvent *event) const;

    /**
     * Process an attempted drop event.
     *
     * Attempts to show a popup menu with possible actions for \p event.
     *
     * \returns True if the event should be further processed, and false otherwise.
     */
    bool processDropEvent(QDropEvent *event, bool &menuCanceled, bool dropOnItem = true);

    /**
     * Starts a drag if possible and sets the appropriate supported actions to allow moves.
     *
     * Also sets the pixmap for the drag to something appropriately small, overriding the Qt
     * behaviour of creating a painting of all selected rows when dragging.
     */
    void startDrag(Qt::DropActions supportedActions);

    /**
     * Sets whether to \p show the drop action menu on drop operation.
     */
    void setShowDropActionMenu(bool show);

    /**
     * Returns whether the drop action menu is shown on drop operation.
     */
    [[nodiscard]] bool showDropActionMenu() const;

    [[nodiscard]] bool isManualSortingActive() const;

    /**
     * Set true if we automatic sorting
     */
    void setManualSortingActive(bool active);

private:
    Collection currentDropTarget(QDropEvent *event) const;

    bool hasAncestor(const QModelIndex &index, Collection::Id parentId) const;
    bool mShowDropActionMenu = true;
    bool mIsManualSortingActive = false;
    QAbstractItemView *const m_view;
};

}
