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
 * The CollectionStatisticsDelegate draws the unread and total count of the
 * underlying CollectionStatisticsModel in a special way.
 *
 * <li>Collections with unread items will have the foldername and the unread
 *     column marked in bold.
 * <li>If a folder is collapsed, the unread and the total column will contain
 *     the total sum of all child folders
 * <li>It has the possibility to draw the unread count directly after the
 *     foldername, see toggleUnreadAfterFolderName().
 */
class AKONADI_EXPORT CollectionStatisticsDelegate : public QStyledItemDelegate
{
  Q_OBJECT

  public:

    /**
     * Constructs an CollectionStatisticsDelegate with the given parent.
     * You still need to call setItemDelegate() yourself.
     *
     * @param parent the parent tree view, which will also take ownership
     */
    explicit CollectionStatisticsDelegate( QTreeView *parent );

    ~CollectionStatisticsDelegate();

  public Q_SLOTS:

    /**
     * Enable or disable the drawing of the unread count behind the folder
     * name.
     * You probably want to enable this when the unread count is hidden only.
     * This is disabled by default.
     *
     * @param enable if true, the unread count is drawn behind the folder name,
     *               if false, the folder name will be drawn normally.
     */
    void setUnreadCountShown( bool enable );

    bool unreadCountShown() const;

  protected:

    /**
     * Reimplemented
     */
    virtual void paint( QPainter *painter, const QStyleOptionViewItem &option,
                        const QModelIndex &index ) const;

    /**
     * Reimplemented
     */
    virtual void initStyleOption( QStyleOptionViewItem *option,
                                  const QModelIndex &index ) const;

  private:
    CollectionStatisticsDelegatePrivate* const d_ptr;

    Q_DECLARE_PRIVATE( CollectionStatisticsDelegate )
};

}

#endif
