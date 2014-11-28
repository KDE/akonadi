/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,
        author Stephen Kelly <stephen@kdab.com>

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

#ifndef AKONADI_ENTITYORDERPROXYMODEL_H
#define AKONADI_ENTITYORDERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include "akonadicore_export.h"

class KConfigGroup;

namespace Akonadi
{
class EntityOrderProxyModelPrivate;

/**
 * @short A model that keeps the order of entities persistent.
 *
 * This proxy maintains the order of entities in a tree. The user can re-order
 * items and the new order will be persisted restored on reset or restart.
 *
 * @author Stephen Kelly <stephen@kdab.com>
 * @since 4.6
 */
class AKONADICORE_EXPORT EntityOrderProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * Creates a new entity order proxy model.
     *
     * @param parent The parent object.
     */
    EntityOrderProxyModel(QObject *parent = 0);

    /**
     * Destroys the entity order proxy model.
     */
    virtual ~EntityOrderProxyModel();

    /**
     * Sets the config @p group that will be used for storing the order.
     */
    void setOrderConfig(KConfigGroup &group);

    /**
     * Saves the order.
     */
    void saveOrder();

    void clearOrder(const QModelIndex &index);
    void clearTreeOrder();

    /**
     * @reimp
     */
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const Q_DECL_OVERRIDE;

    /**
     * @reimp
     */
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) Q_DECL_OVERRIDE;

    /**
     * @reimp
     */
    QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits = 1,
                                  Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const Q_DECL_OVERRIDE;

protected:
    EntityOrderProxyModelPrivate *const d_ptr;

    virtual QString parentConfigString(const QModelIndex &index) const;
    virtual QString configString(const QModelIndex &index) const;

private:
    //@cond PRIVATE
    Q_DECLARE_PRIVATE(EntityOrderProxyModel)
    //@endcond
};

}

#endif
