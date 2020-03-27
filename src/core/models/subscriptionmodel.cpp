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

#include "subscriptionmodel_p.h"
#include "collectionutils.h"
#include "specialcollectionattribute.h"
#include "entityhiddenattribute.h"
#include "entitytreemodel.h"

#include "akonadicore_debug.h"
#include <qnamespace.h>
#include <shared/akranges.h>

#include <QFont>
#include <QSortFilterProxyModel>

using namespace Akonadi;
using namespace AkRanges;

namespace
{

class FilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FilterProxyModel()
        : QSortFilterProxyModel()
    {
        setDynamicSortFilter(true);
    }

    void setShowHidden(bool showHidden)
    {
        if (mShowHidden != showHidden) {
            mShowHidden = showHidden;
            invalidateFilter();
        }
    }

    bool showHidden() const
    {
        return mShowHidden;
    }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        const auto source_index = sourceModel()->index(source_row, 0, source_parent);
        const auto col = source_index.data(EntityTreeModel::CollectionRole).value<Collection>();
        if (mShowHidden) {
            return true;
        }

        return !col.hasAttribute<EntityHiddenAttribute>();
    }

private:
    bool mShowHidden = false;
};

}

/**
 * @internal
 */
class SubscriptionModel::Private
{
public:
    Private(Monitor *monitor)
        : etm(monitor)
    {
        etm.setShowSystemEntities(true); // show hidden collections
        etm.setItemPopulationStrategy(EntityTreeModel::NoItemPopulation);
        etm.setCollectionFetchStrategy(EntityTreeModel::FetchCollectionsRecursive);

        proxy.setSourceModel(&etm);
    }

    Collection::List changedSubscriptions(bool subscribed)
    {
        return Views::range(subscriptions.constKeyValueBegin(), subscriptions.constKeyValueEnd())
                    | Views::filter([subscribed](const auto &val) { return val.second == subscribed; })
                    | Views::transform([](const auto &val) { return Collection{val.first}; })
                    | Actions::toQVector;
    }

    bool isSubscribable(const Collection &col)
    {
        if (CollectionUtils::isStructural(col) || col.isVirtual() || CollectionUtils::isUnifiedMailbox(col)) {
            return false;
        }
        if (col.hasAttribute<SpecialCollectionAttribute>()) {
            return false;
        }
        if (col.contentMimeTypes().isEmpty()) {
            return false;
        }
        return true;
    }

public:
    EntityTreeModel etm;
    FilterProxyModel proxy;
    QHash<Collection::Id, bool> subscriptions;
};

SubscriptionModel::SubscriptionModel(Monitor *monitor, QObject *parent) :
    QIdentityProxyModel(parent),
    d(new Private(monitor))
{
    QIdentityProxyModel::setSourceModel(&d->proxy);

    connect(&d->etm, &EntityTreeModel::collectionTreeFetched, this, &SubscriptionModel::modelLoaded);
}

SubscriptionModel::~SubscriptionModel() = default;

void SubscriptionModel::setSourceModel(QAbstractItemModel *)
{
    // no-op
}

QVariant SubscriptionModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::CheckStateRole: {
        const auto col = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        if (!d->isSubscribable(col)) {
            return QVariant();
        }
        // Check if we have "override" for the subscription state stored
        const auto it = d->subscriptions.constFind(col.id());
        if (it != d->subscriptions.cend()) {
            return (*it) ? Qt::Checked : Qt::Unchecked;
        } else {
            // Fallback to the current state of the collection
            return col.enabled() ? Qt::Checked : Qt::Unchecked;
        }
    }
    case SubscriptionChangedRole: {
        const auto col = index.data(EntityTreeModel::CollectionIdRole).toLongLong();
        return d->subscriptions.contains(col);
    }
    case Qt::FontRole: {
        const auto col = index.data(EntityTreeModel::CollectionIdRole).toLongLong();
        QFont font = QIdentityProxyModel::data(index, role).value<QFont>();
        font.setBold(d->subscriptions.contains(col));
        return font;
    }
    }

    return QIdentityProxyModel::data(index, role);
}

Qt::ItemFlags SubscriptionModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QIdentityProxyModel::flags(index);
    const auto col = index.data(EntityTreeModel::CollectionRole).value<Collection>();
    if (d->isSubscribable(col)) {
        return flags | Qt::ItemIsUserCheckable;
    }
    return flags;
}

bool SubscriptionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::CheckStateRole) {
        const auto col = index.data(EntityTreeModel::CollectionRole).value<Collection>();
        if (!d->isSubscribable(col)) {
            return true; //No change
        }
        if (col.enabled() == (value == Qt::Checked)) { // No change compared to the underlying model
            d->subscriptions.remove(col.id());
        } else {
            d->subscriptions[col.id()] = (value == Qt::Checked);
        }
        Q_EMIT dataChanged(index, index);
        return true;
    }
    return QIdentityProxyModel::setData(index, value, role);
}

Akonadi::Collection::List SubscriptionModel::subscribed() const
{
    return d->changedSubscriptions(true);
}

Akonadi::Collection::List SubscriptionModel::unsubscribed() const
{
    return d->changedSubscriptions(false);
}

void SubscriptionModel::setShowHiddenCollections(bool showHidden)
{
    d->proxy.setShowHidden(showHidden);
}

bool SubscriptionModel::showHiddenCollections() const
{
    return d->proxy.showHidden();
}

#include "subscriptionmodel.moc"

