/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "entitytreemodel.h"

#include <QSortFilterProxyModel>

#include <memory>

namespace Akonadi
{
class EntityRightsFilterModelPrivate;

/*!
 * \brief A proxy model that filters entities by access rights.
 *
 * This class can be used on top of an EntityTreeModel to exclude entities by access type
 * or to include only certain entities with special access rights.
 *
 * \code
 *
 * using namespace Akonadi;
 *
 * EntityTreeModel *model = new EntityTreeModel( this );
 *
 * EntityRightsFilterModel *filter = new EntityRightsFilterModel();
 * filter->setAccessRights( Collection::CanCreateItem | Collection::CanCreateCollection );
 * filter->setSourceModel( model );
 *
 * EntityTreeView *view = new EntityTreeView( this );
 * view->setModel( filter );
 *
 * \endcode
 *
 * @li For collections the access rights are checked against the collections own rights.
 * @li For items the access rights are checked against the item's parent collection rights.
 *
 * \author Tobias Koenig <tokoe@kde.org>
 * \since 4.6
 *
 * \class Akonadi::EntityRightsFilterModel
 * \inheaderfile Akonadi/EntityRightsFilterModel
 * \inmodule AkonadiCore
 */
class AKONADICORE_EXPORT EntityRightsFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /*!
     * Creates a new entity rights filter model.
     *
     * \a parent The parent object.
     */
    explicit EntityRightsFilterModel(QObject *parent = nullptr);

    /*!
     * Destroys the entity rights filter model.
     */
    ~EntityRightsFilterModel() override;

    /*!
     * Sets the access \a rights the entities shall be filtered
     * against. If no rights are set explicitly, Collection::AllRights
     * is assumed.
     * \a rights the access rights filter values
     */
    void setAccessRights(Collection::Rights rights);

    /*!
     * Returns the access rights that are used for filtering.
     */
    [[nodiscard]] Collection::Rights accessRights() const;

    /*!
     * \reimp
     */
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;

    /*!
     * \reimp
     */
    [[nodiscard]] QModelIndexList match(const QModelIndex &start,
                                        int role,
                                        const QVariant &value,
                                        int hits = 1,
                                        Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const override;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    Q_DECLARE_PRIVATE(EntityRightsFilterModel)
    std::unique_ptr<EntityRightsFilterModelPrivate> const d_ptr;
};

}
