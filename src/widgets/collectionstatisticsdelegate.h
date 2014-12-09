/*
    Copyright (c) 2008 Thomas McGuire <thomas.mcguire@gmx.net>

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
#ifndef AKONADI_COLLECTIONSTATISTICSDELEGATE_H
#define AKONADI_COLLECTIONSTATISTICSDELEGATE_H

#include "akonadiwidgets_export.h"

#include <QStyledItemDelegate>

class QAbstractItemView;
class QTreeView;

namespace Akonadi {

class CollectionStatisticsDelegatePrivate;

/**
 * @short A delegate that draws unread and total count for CollectionStatisticsModel.
 *
 * The delegate provides the following features:
 *
 *    - Collections with unread items will have the foldername and the unread
 *      column marked in bold.
 *    - If a folder is collapsed, the unread and the total column will contain
 *      the total sum of all child folders
 *    - It has the possibility to draw the unread count directly after the
 *      foldername, see toggleUnreadAfterFolderName().
 *
 * Example:
 * @code
 *
 * Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView( this );
 *
 * Akonadi::StatisticsProxyModel *statisticsProxy = new Akonadi::StatisticsProxyModel( view );
 * view->setModel( statisticsProxy );
 *
 * Akonadi::CollectionStatisticsDelegate *delegate = new Akonadi::CollectionStatisticsDelegate( view );
 * view->setItemDelegate( delegate );
 *
 * @endcode
 *
 * @note This proxy model is intended to be used on top of the EntityTreeModel. One of the proxies
 * between the EntityTreeModel (the root model) and the view must be a StatisticsProxyModel. That
 * proxy model may appear anywhere in the chain.
 *
 * @author Thomas McGuire <thomas.mcguire@gmx.net>
 */
class AKONADIWIDGETS_EXPORT CollectionStatisticsDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:

    /**
     * Creates a new collection statistics delegate.
     *
     * @param parent The parent item view, which will also take ownership.
     *
     * @since 4.6
     */
    explicit CollectionStatisticsDelegate(QAbstractItemView *parent);

    /**
     * Creates a new collection statistics delegate.
     *
     * @param parent The parent tree view, which will also take ownership.
     */
    explicit CollectionStatisticsDelegate(QTreeView *parent);

    /**
     * Destroys the collection statistics delegate.
     */
    ~CollectionStatisticsDelegate();

    /**
     * @since 4.9.1
     */
    void updatePalette();

public Q_SLOTS:
    /**
     * Sets whether the unread count is drawn next to the folder name.
     *
     * You probably want to enable this when the unread count is hidden only.
     * This is disabled by default.
     *
     * @param enable If @c true, the unread count is drawn next to the folder name,
     *               if @c false, the folder name will be drawn normally.
     */
    void setUnreadCountShown(bool enable);

    /**
     * Returns whether the unread count is drawn next to the folder name.
     */
    bool unreadCountShown() const;

    /**
     * @param enable new mode of progress animation
     */
    void setProgressAnimationEnabled(bool enable);

    bool progressAnimationEnabled() const;

protected:
    /**
     * @param painter pointer for QPainter to use in method
     * @param option style options
     * @param index model index (QModelIndex)
     */
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
                       const QModelIndex &index) const Q_DECL_OVERRIDE;

    /**
     * @param option style option view item
     * @param index model index (QModelIndex)
     */
    void initStyleOption(QStyleOptionViewItem *option,
                                 const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    CollectionStatisticsDelegatePrivate *const d_ptr;
    //@endcond

    Q_DECLARE_PRIVATE(CollectionStatisticsDelegate)
};

}

#endif
