/*
    Copyright (c) 2009 Kevin Ottens <ervin@kde.org>

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

#include "quotacolorproxymodel.h"

#include <entitytreemodel.h>
#include <collectionquotaattribute.h>

#include <QColor>

using namespace Akonadi;

/**
 * @internal
 */
class Q_DECL_HIDDEN QuotaColorProxyModel::Private
{
public:
    Private(QuotaColorProxyModel *parent)
        : mParent(parent)
    {
    }

    QuotaColorProxyModel *mParent = nullptr;

    qreal mThreshold = 100.0;
    QColor mColor = Qt::red;
};

QuotaColorProxyModel::QuotaColorProxyModel(QObject *parent)
    : QIdentityProxyModel(parent),
      d(new Private(this))
{
}

QuotaColorProxyModel::~QuotaColorProxyModel()
{
    delete d;
}

void QuotaColorProxyModel::setWarningThreshold(qreal threshold)
{
    d->mThreshold = threshold;
}

qreal QuotaColorProxyModel::warningThreshold() const
{
    return d->mThreshold;
}

void QuotaColorProxyModel::setWarningColor(const QColor &color)
{
    d->mColor = color;
}

QColor QuotaColorProxyModel::warningColor() const
{
    return d->mColor;
}

QVariant QuotaColorProxyModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::ForegroundRole) {
        const QModelIndex sourceIndex = mapToSource(index);
        const QModelIndex rowIndex = sourceIndex.sibling(sourceIndex.row(), 0);
        const Akonadi::Collection collection = sourceModel()->data(rowIndex, Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();

        if (collection.isValid() && collection.hasAttribute<Akonadi::CollectionQuotaAttribute>()) {
            const Akonadi::CollectionQuotaAttribute *quota = collection.attribute<Akonadi::CollectionQuotaAttribute>();

            if (quota->currentValue() > -1 && quota->maximumValue() > 0) {
                const qreal percentage = (100.0 * quota->currentValue()) / quota->maximumValue();

                if (percentage >= d->mThreshold) {
                    return d->mColor;
                }
            }
        }
    }

    return QIdentityProxyModel::data(index, role);
}
