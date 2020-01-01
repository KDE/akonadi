/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>
    Copyright (C) 2012-2020 Laurent Montel <montel@kde.org>

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

#include "recursivecollectionfilterproxymodel.h"

#include "entitytreemodel.h"
#include "mimetypechecker.h"


using namespace Akonadi;

namespace Akonadi
{

class RecursiveCollectionFilterProxyModelPrivate
{
    Q_DECLARE_PUBLIC(RecursiveCollectionFilterProxyModel)
    RecursiveCollectionFilterProxyModel *q_ptr;
public:
    RecursiveCollectionFilterProxyModelPrivate(RecursiveCollectionFilterProxyModel *model)
        : q_ptr(model)
    {

    }

    QSet<QString> includedMimeTypes;
    Akonadi::MimeTypeChecker checker;
    QString pattern;
    bool checkOnlyChecked = false;
};

}

RecursiveCollectionFilterProxyModel::RecursiveCollectionFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new RecursiveCollectionFilterProxyModelPrivate(this))
{
    setRecursiveFilteringEnabled(true);
}

RecursiveCollectionFilterProxyModel::~RecursiveCollectionFilterProxyModel()
{
    delete d_ptr;
}

bool RecursiveCollectionFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_D(const RecursiveCollectionFilterProxyModel);

    const QModelIndex rowIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    const Akonadi::Collection collection = rowIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    if (!collection.isValid()) {
        return false;
    }
    const bool checked = (rowIndex.data(Qt::CheckStateRole).toInt() == Qt::Checked);
    const bool isCheckable = sourceModel()->flags(rowIndex) & Qt::ItemIsUserCheckable;
    if (isCheckable && (d->checkOnlyChecked && !checked)) {
        return false;
    }

    const bool collectionWanted =  d->checker.isWantedCollection(collection);
    if (collectionWanted) {
        if (!d->pattern.isEmpty()) {
            const QString text = rowIndex.data(Qt::DisplayRole).toString();
            return text.contains(d->pattern, Qt::CaseInsensitive);
        }
    }
    return collectionWanted;
}


void RecursiveCollectionFilterProxyModel::addContentMimeTypeInclusionFilter(const QString &mimeType)
{
    Q_D(RecursiveCollectionFilterProxyModel);
    d->includedMimeTypes << mimeType;
    d->checker.setWantedMimeTypes(d->includedMimeTypes.values());
    invalidateFilter();
}

void RecursiveCollectionFilterProxyModel::addContentMimeTypeInclusionFilters(const QStringList &mimeTypes)
{
    Q_D(RecursiveCollectionFilterProxyModel);
    d->includedMimeTypes.unite(mimeTypes.toSet());
    d->checker.setWantedMimeTypes(d->includedMimeTypes.values());
    invalidateFilter();
}

void RecursiveCollectionFilterProxyModel::clearFilters()
{
    Q_D(RecursiveCollectionFilterProxyModel);
    d->includedMimeTypes.clear();
    d->checker.setWantedMimeTypes(QStringList());
    invalidateFilter();
}

void RecursiveCollectionFilterProxyModel::setContentMimeTypeInclusionFilters(const QStringList &mimeTypes)
{
    Q_D(RecursiveCollectionFilterProxyModel);
    d->includedMimeTypes = mimeTypes.toSet();
    d->checker.setWantedMimeTypes(d->includedMimeTypes.values());
    invalidateFilter();
}

QStringList RecursiveCollectionFilterProxyModel::contentMimeTypeInclusionFilters() const
{
    Q_D(const RecursiveCollectionFilterProxyModel);
    return d->includedMimeTypes.values();
}

int Akonadi::RecursiveCollectionFilterProxyModel::columnCount(const QModelIndex &index) const
{
    // Optimization: we know that we're not changing the number of columns, so skip QSortFilterProxyModel
    return sourceModel()->columnCount(mapToSource(index));
}

void Akonadi::RecursiveCollectionFilterProxyModel::setSearchPattern(const QString &pattern)
{
    Q_D(RecursiveCollectionFilterProxyModel);
    if (d->pattern != pattern) {
        d->pattern = pattern;
        invalidate();
    }
}

void Akonadi::RecursiveCollectionFilterProxyModel::setIncludeCheckedOnly(bool checked)
{
    Q_D(RecursiveCollectionFilterProxyModel);
    if (d->checkOnlyChecked != checked) {
        d->checkOnlyChecked = checked;
        invalidate();
    }
}
