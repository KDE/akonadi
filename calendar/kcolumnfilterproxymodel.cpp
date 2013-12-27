/*
    Copyright (C) 2010 Klarälvdalens Datakonsult AB,
        a KDAB Group company, info@kdab.net,

    author Bertjan Broeksema <broeksema@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "kcolumnfilterproxymodel_p.h"

#include <QtCore/QVector>

using namespace Akonadi;

namespace Akonadi {
class KColumnFilterProxyModelPrivate
{
public:
    QVector<int> m_visibleColumns;
};
}

KColumnFilterProxyModel::KColumnFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new KColumnFilterProxyModelPrivate)
{ }

KColumnFilterProxyModel::~KColumnFilterProxyModel()
{
    delete d_ptr;
}

QVector<int> KColumnFilterProxyModel::visbileColumns() const
{
    Q_D(const KColumnFilterProxyModel);
    return d->m_visibleColumns;
}

void KColumnFilterProxyModel::setVisibleColumn(int column)
{
    setVisibleColumns(QVector<int>() << column);
}

void KColumnFilterProxyModel::setVisibleColumns(const QVector<int> &visibleColumns)
{
    Q_D(KColumnFilterProxyModel);
    d->m_visibleColumns = visibleColumns;
    invalidateFilter();
}

bool KColumnFilterProxyModel::filterAcceptsColumn(int column, const QModelIndex &parent) const
{
    Q_D(const KColumnFilterProxyModel);

    if (!d->m_visibleColumns.isEmpty() && !d->m_visibleColumns.contains(column)) {
        // We only filter columns out when m_visibleColumns actually contains values.
        return false;
    }

    return QSortFilterProxyModel::filterAcceptsColumn(column, parent);
}

