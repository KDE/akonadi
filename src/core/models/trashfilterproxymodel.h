/*
 SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

 SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QSortFilterProxyModel>

namespace Akonadi
{
/**
 * @short Filter model which hides/shows entities marked as trash
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
class AKONADICORE_EXPORT TrashFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TrashFilterProxyModel(QObject *parent = nullptr);
    ~TrashFilterProxyModel() override;

    void showTrash(bool enable);
    Q_REQUIRED_RESULT bool trashIsShown() const;

protected:
    /**
     * Sort filter criteria, according to how expensive the operation is
     */
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    /// @cond PRIVATE
    class TrashFilterProxyModelPrivate;
    TrashFilterProxyModelPrivate *const d_ptr;
    Q_DECLARE_PRIVATE(TrashFilterProxyModel)
    /// @endcond
};

}

