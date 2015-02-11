/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SUBSCRIPTIONMODEL_P_H
#define AKONADI_SUBSCRIPTIONMODEL_P_H

#include "akonadicore_export.h"
#include "collection.h"
#include "collectionmodel.h"

namespace Akonadi {

/**
 * @internal
 * @deprecated This should be replaced by something based on EntityTreeModel
 *
 * An extended collection model used for the subscription dialog.
 */
class AKONADICORE_EXPORT SubscriptionModel : public CollectionModel
{
    Q_OBJECT
public:
    /** Additional roles. */
    enum Roles {
        SubscriptionChangedRole = CollectionModel::UserRole + 1 ///< Indicate the subscription status has been changed.
    };

    /**
      Create a new subscription model.
      @param parent The parent object.
    */
    explicit SubscriptionModel(QObject *parent = 0);

    /**
      Destructor.
    */
    ~SubscriptionModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;

    Collection::List subscribed() const;
    Collection::List unsubscribed() const;

    /**
     * @param showHidden shows hidden collection if set as @c true
     * @since: 4.9
     */
    void showHiddenCollection(bool showHidden);

Q_SIGNALS:
    /**
      Emitted when the collection model is fully loaded.
    */
    void loaded();

private:
    class Private;
    Private *const d;
    Q_PRIVATE_SLOT(d, void listResult(KJob *))
};

}

#endif
