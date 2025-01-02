/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>
    SPDX-FileCopyrightText: 2012-2025 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "recursivecollectionfilterproxymodel.h"

#include "collectionutils.h"
#include "entitytreemodel.h"
#include "mimetypechecker.h"

using namespace Akonadi;

namespace Akonadi
{
class RecursiveCollectionFilterProxyModelPrivate
{
    Q_DECLARE_PUBLIC(RecursiveCollectionFilterProxyModel)
    RecursiveCollectionFilterProxyModel *const q_ptr;

public:
    explicit RecursiveCollectionFilterProxyModelPrivate(RecursiveCollectionFilterProxyModel *model)
        : q_ptr(model)
    {
    }

    QSet<QString> includedMimeTypes;
    Akonadi::MimeTypeChecker checker;
    QString pattern;
    bool checkOnlyChecked = false;
    bool excludeUnifiedMailBox = false;
};

} // namespace Akonadi

RecursiveCollectionFilterProxyModel::RecursiveCollectionFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d_ptr(new RecursiveCollectionFilterProxyModelPrivate(this))
{
    setRecursiveFilteringEnabled(true);
}

RecursiveCollectionFilterProxyModel::~RecursiveCollectionFilterProxyModel() = default;

bool RecursiveCollectionFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    Q_D(const RecursiveCollectionFilterProxyModel);

    const QModelIndex rowIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto collection = rowIndex.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    if (!collection.isValid()) {
        return false;
    }
    if (CollectionUtils::isUnifiedMailbox(collection)) {
        return false;
    }
    const bool checked = (rowIndex.data(Qt::CheckStateRole).toInt() == Qt::Checked);
    const bool isCheckable = sourceModel()->flags(rowIndex) & Qt::ItemIsUserCheckable;
    if (isCheckable && (d->checkOnlyChecked && !checked)) {
        return false;
    }

    const bool collectionWanted = d->checker.isWantedCollection(collection);
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
    d->includedMimeTypes.unite(QSet<QString>(mimeTypes.begin(), mimeTypes.end()));
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
    d->includedMimeTypes = QSet<QString>(mimeTypes.begin(), mimeTypes.end());
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

void RecursiveCollectionFilterProxyModel::setExcludeUnifiedMailBox(bool exclude)
{
    Q_D(RecursiveCollectionFilterProxyModel);
    if (d->excludeUnifiedMailBox != exclude) {
        d->excludeUnifiedMailBox = exclude;
        invalidate();
    }
}

#include "moc_recursivecollectionfilterproxymodel.cpp"
