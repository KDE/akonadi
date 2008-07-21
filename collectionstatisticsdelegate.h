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

#include "akonadi_export.h"

#include <QtGui/QStyledItemDelegate>

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
 * QTreeView *view = new QTreeView( this );
 *
 * Akonadi::CollectionStatisticsModel *model = new Akonadi::CollectionStatisticsModel( view );
 * view->setModel( model );
 *
 * Akonadi::CollectionStatisticsDelegate *delegate = new Akonadi::CollectionStatisticsDelegate( view );
 * view->setItemDelegate( delegate );
 *
 * @endcode
 *
 * @author Thomas McGuire <thomas.mcguire@gmx.net>
 */
class AKONADI_EXPORT CollectionStatisticsDelegate : public QStyledItemDelegate
{
  Q_OBJECT

  public:

    /**
     * Creates a new collection statistics delegate.
     *
     * @param parent The parent tree view, which will also take ownership.
     */
    explicit CollectionStatisticsDelegate( QTreeView *parent );

    /**
     * Destroys the collection statistics delegate.
     */
    ~CollectionStatisticsDelegate();

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
    void setUnreadCountShown( bool enable );

    /**
     * Returns whether the unread count is drawn next to the folder name.
     */
    bool unreadCountShown() const;

  protected:
    virtual void paint( QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index ) const;

    virtual void initStyleOption( QStyleOptionViewItem *option,
                                  const QModelIndex &index ) const;

  private:
    //@cond PRIVATE
    CollectionStatisticsDelegatePrivate* const d_ptr;
    //@endcond

    Q_DECLARE_PRIVATE( CollectionStatisticsDelegate )
};

}

#endif
