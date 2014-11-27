/*
 Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#ifndef AKONADI_TRASHFILTERPROXYMODEL_H
#define AKONADI_TRASHFILTERPROXYMODEL_H

#include "akonadicore_export.h"

#include <krecursivefilterproxymodel.h>

namespace Akonadi
{

/**
 * @short Filter model which hides/shows entites marked as trash
 *
 * Filter model which either hides all entities marked as trash, or the ones not marked.
 * Subentities of collections marked as trash are also shown in the trash.
 *
 * The Base model must be an EntityTreeModel and the EntityDeletedAttribute must be available.
 *
 * Example:
 *
 * @code
 *
 * ChangeRecorder *monitor = new Akonadi::ChangeRecorder( this );
 * monitor->itemFetchScope().fetchAttribute<Akonadi::EntityDisplayAttribute>(true);
 *
 * Akonadi::EntityTreeModel *sourcemodel = new Akonadi::EntityTreeModel(monitor, this);
 *
 * TrashFilterProxyModel *model = new TrashFilterProxyModel(this);
 * model->setDynamicSortFilter(true);
 * model->setSourceModel(sourcemodel);
 *
 * @endcode
 *
 * @author Christian Mollekopf <chrigi_1@fastmail.fm>
 * @since 4.8
 */
class AKONADICORE_EXPORT TrashFilterProxyModel : public KRecursiveFilterProxyModel
{
    Q_OBJECT

public:
    explicit TrashFilterProxyModel(QObject *parent = 0);
    virtual ~TrashFilterProxyModel();

    void showTrash(bool enable);
    bool trashIsShown() const;

protected:
    /**
     * Sort filter criterias, according to how expensive the operation is
     */
    bool acceptRow(int sourceRow, const QModelIndex &sourceParent) const Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class TrashFilterProxyModelPrivate;
    TrashFilterProxyModelPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(TrashFilterProxyModel)
    //@endcond
};

}

#endif
