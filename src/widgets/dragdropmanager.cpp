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

#include "dragdropmanager_p.h"
#include "specialcollectionattribute.h"
#include "collectionutils.h"

#include <QApplication>
#include <QDropEvent>
#include <QMenu>
#include <QDrag>

#include <KLocalizedString>
#include <KUrl>

#include "collection.h"
#include "entitytreemodel.h"

using namespace Akonadi;

DragDropManager::DragDropManager(QAbstractItemView *view)
    : mShowDropActionMenu(true)
    , mIsManualSortingActive(false)
    , m_view(view)
{
}

Akonadi::Collection DragDropManager::currentDropTarget(QDropEvent *event) const
{
    const QModelIndex index = m_view->indexAt(event->pos());
    Collection collection = m_view->model()->data(index, EntityTreeModel::CollectionRole).value<Collection>();
    if (!collection.isValid()) {
        const Item item = m_view->model()->data(index, EntityTreeModel::ItemRole).value<Item>();
        if (item.isValid()) {
            collection = m_view->model()->data(index.parent(), EntityTreeModel::CollectionRole).value<Collection>();
        }
    }

    return collection;
}

bool DragDropManager::dropAllowed(QDragMoveEvent *event) const
{
    // Check if the collection under the cursor accepts this data type
    const Collection targetCollection = currentDropTarget(event);
    if (targetCollection.isValid()) {
        const QStringList supportedContentTypes = targetCollection.contentMimeTypes();

        const QMimeData *data = event->mimeData();
        const KUrl::List urls = KUrl::List::fromMimeData(data);
        foreach (const KUrl &url, urls) {
            const Collection collection = Collection::fromUrl(url);
            if (collection.isValid()) {
                if (!supportedContentTypes.contains(Collection::mimeType()) &&
                    !supportedContentTypes.contains(Collection::virtualMimeType())) {
                    break;
                }

                // Check if we don't try to drop on one of the children
                if (hasAncestor(m_view->indexAt(event->pos()), collection.id())) {
                    break;
                }
            } else { // This is an item.
                const QString type = url.queryItems()[QString::fromLatin1("type")];
                if (!supportedContentTypes.contains(type)) {
                    break;
                }
            }

            return true;
        }
    }

    return false;
}

bool DragDropManager::hasAncestor(const QModelIndex &_index, Collection::Id parentId) const
{
    QModelIndex index(_index);
    while (index.isValid()) {
        if (m_view->model()->data(index, EntityTreeModel::CollectionIdRole).toLongLong() == parentId) {
            return true;
        }

        index = index.parent();
    }

    return false;
}

bool DragDropManager::processDropEvent(QDropEvent *event, bool &menuCanceled, bool dropOnItem)
{
    const Collection targetCollection = currentDropTarget(event);
    if (!targetCollection.isValid()) {
        return false;
    }

    if (!mIsManualSortingActive && !dropOnItem) {
        return false;
    }

    const QStringList supportedContentTypes = targetCollection.contentMimeTypes();

    const QMimeData *data = event->mimeData();
    const KUrl::List urls = KUrl::List::fromMimeData(data);
    foreach (const KUrl &url, urls) {
        const Collection collection = Collection::fromUrl(url);
        if (!collection.isValid()) {
            if (!dropOnItem) {
                return false;
            }
        }
    }

    int actionCount = 0;
    Qt::DropAction defaultAction;
    // TODO check if the source supports moving

    bool moveAllowed, copyAllowed, linkAllowed;
    moveAllowed = copyAllowed = linkAllowed = false;

    if ((targetCollection.rights() & (Collection::CanCreateCollection | Collection::CanCreateItem)) &&
        (event->possibleActions() & Qt::MoveAction)) {
        moveAllowed = true;
    }
    if ((targetCollection.rights() & (Collection::CanCreateCollection | Collection::CanCreateItem)) &&
        (event->possibleActions() & Qt::CopyAction)) {
        copyAllowed = true;
    }

    if ((targetCollection.rights() & Collection::CanLinkItem) &&
        (event->possibleActions() & Qt::LinkAction)) {
        linkAllowed = true;
    }

    if (mIsManualSortingActive && !dropOnItem) {
        moveAllowed = true;
        copyAllowed = false;
        linkAllowed = false;
    }

    if (!moveAllowed && !copyAllowed && !linkAllowed) {
        qDebug() << "Cannot drop here:" << event->possibleActions() << m_view->model()->supportedDragActions() << m_view->model()->supportedDropActions();
        return false;
    }

    // first check whether the user pressed a modifier key to select a specific action
    if ((QApplication::keyboardModifiers() & Qt::ControlModifier) &&
        (QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
        if (linkAllowed) {
            defaultAction = Qt::LinkAction;
            actionCount = 1;
        } else {
            return false;
        }
    } else if ((QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        if (copyAllowed) {
            defaultAction = Qt::CopyAction;
            actionCount = 1;
        } else {
            return false;
        }
    } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
        if (moveAllowed) {
            defaultAction = Qt::MoveAction;
            actionCount = 1;
        } else {
            return false;
        }
    }

    if (actionCount == 1) {
        qDebug() << "Selecting drop action" << defaultAction << ", there are no other possibilities";
        event->setDropAction(defaultAction);
        return true;
    }

    if (!mShowDropActionMenu) {
        if (moveAllowed) {
            defaultAction = Qt::MoveAction;
        } else if (copyAllowed) {
            defaultAction = Qt::CopyAction;
        } else if (linkAllowed) {
            defaultAction = Qt::LinkAction;
        } else {
            return false;
        }
        event->setDropAction(defaultAction);
        return true;
    }

    // otherwise show up a menu to allow the user to select an action
    QMenu popup(m_view);
    QAction *moveDropAction = 0;
    QAction *copyDropAction = 0;
    QAction *linkAction = 0;
    QString sequence;

    if (moveAllowed) {
        sequence = QKeySequence(Qt::ShiftModifier).toString();
        sequence.chop(1);   // chop superfluous '+'
        moveDropAction = popup.addAction(QIcon::fromTheme(QString::fromLatin1("go-jump")), i18n("&Move Here") + QLatin1Char('\t') + sequence);
    }

    if (copyAllowed) {
        sequence = QKeySequence(Qt::ControlModifier).toString();
        sequence.chop(1);   // chop superfluous '+'
        copyDropAction = popup.addAction(QIcon::fromTheme(QString::fromLatin1("edit-copy")), i18n("&Copy Here") + QLatin1Char('\t') + sequence);
    }

    if (linkAllowed) {
        sequence = QKeySequence(Qt::ControlModifier + Qt::ShiftModifier).toString();
        sequence.chop(1);   // chop superfluous '+'
        linkAction = popup.addAction(QIcon::fromTheme(QStringLiteral("edit-link")), i18n("&Link Here") + QLatin1Char('\t') + sequence);
    }

    popup.addSeparator();
    popup.addAction(QIcon::fromTheme(QString::fromLatin1("process-stop")), i18n("C&ancel") + QLatin1Char('\t') + QKeySequence(Qt::Key_Escape).toString());

    QAction *activatedAction = popup.exec(QCursor::pos());
    if (!activatedAction) {
        menuCanceled = true;
        return false;
    } else if (activatedAction == moveDropAction) {
        event->setDropAction(Qt::MoveAction);
    } else if (activatedAction == copyDropAction) {
        event->setDropAction(Qt::CopyAction);
    } else if (activatedAction == linkAction) {
        event->setDropAction(Qt::LinkAction);
    } else {
        menuCanceled = true;
        return false;
    }
    return true;
}

void DragDropManager::startDrag(Qt::DropActions supportedActions)
{
    QModelIndexList indexes;
    bool sourceDeletable = true;
    foreach (const QModelIndex &index, m_view->selectionModel()->selectedRows()) {
        if (!m_view->model()->flags(index).testFlag(Qt::ItemIsDragEnabled)) {
            continue;
        }

        if (sourceDeletable) {
            Collection source = index.data(EntityTreeModel::CollectionRole).value<Collection>();
            if (!source.isValid()) {
                // index points to an item
                source = index.data(EntityTreeModel::ParentCollectionRole).value<Collection>();
                sourceDeletable = source.rights() & Collection::CanDeleteItem;
            } else {
                // index points to a collection
                sourceDeletable = (source.rights() & Collection::CanDeleteCollection) && !source.hasAttribute<SpecialCollectionAttribute>() && !source.isVirtual();
            }
        }
        indexes.append(index);
    }

    if (indexes.isEmpty()) {
        return;
    }

    QMimeData *mimeData = m_view->model()->mimeData(indexes);
    if (!mimeData) {
        return;
    }

    QDrag *drag = new QDrag(m_view);
    drag->setMimeData(mimeData);
    if (indexes.size() > 1) {
        drag->setPixmap(QIcon::fromTheme(QStringLiteral("document-multiple")).pixmap(QSize(22, 22)));
    } else {
        QPixmap pixmap = indexes.first().data(Qt::DecorationRole).value<QIcon>().pixmap(QSize(22, 22));
        if (pixmap.isNull()) {
            pixmap = QIcon::fromTheme(QStringLiteral("text-plain")).pixmap(QSize(22, 22));
        }
        drag->setPixmap(pixmap);
    }

    if (!sourceDeletable) {
        supportedActions &= ~Qt::MoveAction;
    }

    Qt::DropAction defaultAction = Qt::IgnoreAction;
    if ((QApplication::keyboardModifiers() & Qt::ControlModifier) &&
        (QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
        defaultAction = Qt::LinkAction;
    } else if ((QApplication::keyboardModifiers() & Qt::ControlModifier)) {
        defaultAction = Qt::CopyAction;
    } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier)) {
        defaultAction = Qt::MoveAction;
    }

    drag->exec(supportedActions, defaultAction);
}

bool DragDropManager::showDropActionMenu() const
{
    return mShowDropActionMenu;
}

void DragDropManager::setShowDropActionMenu(bool show)
{
    mShowDropActionMenu = show;
}

bool DragDropManager::isManualSortingActive() const
{
    return mIsManualSortingActive;
}

void DragDropManager::setManualSortingActive(bool active)
{
    mIsManualSortingActive = active;
}
