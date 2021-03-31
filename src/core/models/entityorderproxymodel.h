/*
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net
    SPDX-FileContributor: Stephen Kelly <stephen@kdab.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "collection.h"

#include <QSortFilterProxyModel>

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
    explicit EntityOrderProxyModel(QObject *parent = nullptr);

    /**
     * Destroys the entity order proxy model.
     */
    ~EntityOrderProxyModel() override;

    /**
     * Sets the config @p group that will be used for storing the order.
     */
    void setOrderConfig(const KConfigGroup &group);

    /**
     * Saves the order.
     */
    void saveOrder();

    void clearOrder(const QModelIndex &index);
    void clearTreeOrder();

    /**
     * @reimp
     */
    Q_REQUIRED_RESULT bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

    /**
     * @reimp
     */
    Q_REQUIRED_RESULT bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;

    /**
     * @reimp
     */
    Q_REQUIRED_RESULT QModelIndexList match(const QModelIndex &start,
                                            int role,
                                            const QVariant &value,
                                            int hits = 1,
                                            Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const override;

protected:
    EntityOrderProxyModelPrivate *const d_ptr;

    virtual QString parentConfigString(const QModelIndex &index) const;
    virtual QString configString(const QModelIndex &index) const;
    virtual Akonadi::Collection parentCollection(const QModelIndex &index) const;

private:
    QStringList configStringsForDroppedUrls(const QList<QUrl> &urls, const Akonadi::Collection &parentCol, bool *containsMove) const;

    /// @cond PRIVATE
    Q_DECLARE_PRIVATE(EntityOrderProxyModel)
    /// @endcond
};

}

