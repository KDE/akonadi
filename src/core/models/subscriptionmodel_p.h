/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QIdentityProxyModel>

#include "entitytreemodel.h"

#include <memory>

namespace Akonadi
{
class Monitor;

/**
 * @internal
 *
 * A proxy model to be used on top of ETM to display a checkable tree of collections
 * for user to select which collections should be locally subscribed.
 *
 * Used in SubscriptionDialog
 */
class AKONADICORE_EXPORT SubscriptionModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    /** Additional roles. */
    enum Roles {
        SubscriptionChangedRole = EntityTreeModel::UserRole + 1 ///< Indicate the subscription status has been changed.
    };

    /**
      Create a new subscription model.
      @param parent The parent object.
    */
    explicit SubscriptionModel(Monitor *monitor, QObject *parent = nullptr);

    /**
      Destructor.
    */
    ~SubscriptionModel() override;

    /**
     * Sets a source model for the SubscriptionModel.
     *
     * Should be based on an ETM with only collections.
     */
    void setSourceModel(QAbstractItemModel *model) override;

    Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_REQUIRED_RESULT Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    Q_REQUIRED_RESULT Collection::List subscribed() const;
    Q_REQUIRED_RESULT Collection::List unsubscribed() const;

    /**
     * @param showHidden shows hidden collections if set as @c true
     * @since: 4.9
     */
    void setShowHiddenCollections(bool showHidden);
    Q_REQUIRED_RESULT bool showHiddenCollections() const;

Q_SIGNALS:
    void modelLoaded();

private:
    class Private;
    std::unique_ptr<Private> const d;
};

}

