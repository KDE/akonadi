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
class SubscriptionModelPrivate;

/*!
 * \internal
 *
 * A proxy model to be used on top of ETM to display a checkable tree of collections
 * for user to select which collections should be locally subscribed.
 *
 * \class Akonadi::SubscriptionModel
 * \inheaderfile Akonadi/SubscriptionModel
 * \inmodule AkonadiCore
 *
 * Used in SubscriptionDialog
 */
class AKONADICORE_EXPORT SubscriptionModel : public QIdentityProxyModel
{
    Q_OBJECT
public:
    /*! Additional roles. */
    enum Roles {
        SubscriptionChangedRole = EntityTreeModel::UserRole + 1 ///< Indicate the subscription status has been changed.
    };

    /*!
      Create a new subscription model.
      \ parent The parent object.
    */
    explicit SubscriptionModel(Monitor *monitor, QObject *parent = nullptr);

    /*!
      Destructor.
    */
    ~SubscriptionModel() override;

    /*!
     * Sets a source model for the SubscriptionModel.
     *
     * Should be based on an ETM with only collections.
     */
    void setSourceModel(QAbstractItemModel *model) override;

    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    [[nodiscard]] Collection::List subscribed() const;
    [[nodiscard]] Collection::List unsubscribed() const;

    /*!
     * \ showHidden shows hidden collections if set as \\ true
     * \since: 4.9
     */
    void setShowHiddenCollections(bool showHidden);
    [[nodiscard]] bool showHiddenCollections() const;

Q_SIGNALS:
    void modelLoaded();

private:
    std::unique_ptr<SubscriptionModelPrivate> const d;
};

}
