/*
    SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno.virlet@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionfilterproxymodel.h"
#include "akonadicore_debug.h"
#include "entitytreemodel.h"
#include "mimetypechecker.h"

#include <QString>
#include <QStringList>

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::CollectionFilterProxyModelPrivate
{
public:
    explicit CollectionFilterProxyModelPrivate(CollectionFilterProxyModel *parent)
        : mParent(parent)
    {
        mimeChecker.addWantedMimeType(QStringLiteral("text/uri-list"));
    }

    bool collectionAccepted(const QModelIndex &index, bool checkResourceVisibility = true);

    QVector<QModelIndex> acceptedResources;
    CollectionFilterProxyModel *const mParent;
    MimeTypeChecker mimeChecker;
    bool mExcludeVirtualCollections = false;
};

bool CollectionFilterProxyModelPrivate::collectionAccepted(const QModelIndex &index, bool checkResourceVisibility)
{
    // Retrieve supported mimetypes
    const auto collection = mParent->sourceModel()->data(index, EntityTreeModel::CollectionRole).value<Collection>();

    if (!collection.isValid()) {
        return false;
    }

    if (collection.isVirtual() && mExcludeVirtualCollections) {
        return false;
    }

    // If this collection directly contains one valid mimetype, it is accepted
    if (mimeChecker.isWantedCollection(collection)) {
        // The folder will be accepted, but we need to make sure the resource is visible too.
        if (checkResourceVisibility) {
            // find the resource
            QModelIndex resource = index;
            while (resource.parent().isValid()) {
                resource = resource.parent();
            }

            // See if that resource is visible, if not, invalidate the filter.
            if (resource != index && !acceptedResources.contains(resource)) {
                qCDebug(AKONADICORE_LOG) << "We got a new collection:" << mParent->sourceModel()->data(index).toString()
                                         << "but the resource is not visible:" << mParent->sourceModel()->data(resource).toString();
                acceptedResources.clear();
                // defer reset, the model might still be supplying new items at this point which crashes
                mParent->invalidateFilter();
                return true;
            }
        }

        // Keep track of all the resources that are visible.
        if (!index.parent().isValid()) {
            acceptedResources.append(index);
        }

        return true;
    }

    // If this collection has a child which contains valid mimetypes, it is accepted
    QModelIndex childIndex = mParent->sourceModel()->index(0, 0, index);
    while (childIndex.isValid()) {
        if (collectionAccepted(childIndex, false /* don't check visibility of the parent, as we are checking the child now */)) {
            // Keep track of all the resources that are visible.
            if (!index.parent().isValid()) {
                acceptedResources.append(index);
            }

            return true;
        }
        childIndex = childIndex.sibling(childIndex.row() + 1, 0);
    }

    // Or else, no reason to keep this collection.
    return false;
}

CollectionFilterProxyModel::CollectionFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new CollectionFilterProxyModelPrivate(this))
{
}

CollectionFilterProxyModel::~CollectionFilterProxyModel() = default;

void CollectionFilterProxyModel::addMimeTypeFilters(const QStringList &typeList)
{
    const QStringList mimeTypes = d->mimeChecker.wantedMimeTypes() + typeList;
    d->mimeChecker.setWantedMimeTypes(mimeTypes);
    invalidateFilter();
}

void CollectionFilterProxyModel::addMimeTypeFilter(const QString &type)
{
    d->mimeChecker.addWantedMimeType(type);
    invalidateFilter();
}

bool CollectionFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return d->collectionAccepted(sourceModel()->index(sourceRow, 0, sourceParent));
}

QStringList CollectionFilterProxyModel::mimeTypeFilters() const
{
    return d->mimeChecker.wantedMimeTypes();
}

void CollectionFilterProxyModel::clearFilters()
{
    d->mimeChecker = MimeTypeChecker();
    invalidateFilter();
}

void CollectionFilterProxyModel::setExcludeVirtualCollections(bool exclude)
{
    if (exclude != d->mExcludeVirtualCollections) {
        d->mExcludeVirtualCollections = exclude;
        invalidateFilter();
    }
}

bool CollectionFilterProxyModel::excludeVirtualCollections() const
{
    return d->mExcludeVirtualCollections;
}

Qt::ItemFlags CollectionFilterProxyModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        // Don't crash
        return Qt::NoItemFlags;
    }

    const auto collection = sourceModel()->data(mapToSource(index), EntityTreeModel::CollectionRole).value<Collection>();

    // If this collection directly contains one valid mimetype, it is accepted
    if (d->mimeChecker.isWantedCollection(collection)) {
        return QSortFilterProxyModel::flags(index);
    } else {
        return QSortFilterProxyModel::flags(index) & ~(Qt::ItemIsSelectable);
    }
}
