/*
    SPDX-FileCopyrightText: 2009 Kevin Ottens <ervin@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "favoritecollectionsmodel.h"
#include "akonadicore_debug.h"

#include <QItemSelectionModel>
#include <QMimeData>

#include <KConfig>
#include <KConfigGroup>
#include <KJob>
#include <KLocalizedString>
#include <QUrl>

#include "collectionmodifyjob.h"
#include "entitytreemodel.h"
#include "favoritecollectionattribute.h"
#include "mimetypechecker.h"
#include "pastehelper_p.h"

using namespace Akonadi;

/**
 * @internal
 */
class Akonadi::FavoriteCollectionsModelPrivate
{
public:
    FavoriteCollectionsModelPrivate(const KConfigGroup &group, FavoriteCollectionsModel *parent)
        : q(parent)
        , configGroup(group)
    {
    }

    QString labelForCollection(Collection::Id collectionId) const
    {
        if (labelMap.contains(collectionId)) {
            return labelMap[collectionId];
        }

        return q->defaultFavoriteLabel(Collection{collectionId});
    }

    void insertIfAvailable(Collection::Id col)
    {
        if (collectionIds.contains(col)) {
            select(col);
            if (!referencedCollections.contains(col)) {
                reference(col);
            }
            auto idx = EntityTreeModel::modelIndexForCollection(q, Collection{col});
            if (idx.isValid()) {
                auto c = q->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
                if (c.isValid() && !c.hasAttribute<FavoriteCollectionAttribute>()) {
                    c.addAttribute(new FavoriteCollectionAttribute());
                    new CollectionModifyJob(c, q);
                }
            }
        }
    }

    void insertIfAvailable(const QModelIndex &idx)
    {
        insertIfAvailable(idx.data(EntityTreeModel::CollectionIdRole).value<Collection::Id>());
    }

    /**
     * Stuff changed (e.g. new rows inserted into sorted model), reload everything.
     */
    void reload()
    {
        // don't clear the selection model here. Otherwise we mess up the users selection as collections get removed and re-inserted.
        for (const Collection::Id &collectionId : std::as_const(collectionIds)) {
            insertIfAvailable(collectionId);
        }
        // If a favorite folder was removed then surely it's gone from the selection model, so no need to do anything about that.
    }

    void rowsInserted(const QModelIndex &parent, int begin, int end)
    {
        for (int row = begin; row <= end; row++) {
            const QModelIndex child = q->sourceModel()->index(row, 0, parent);
            if (!child.isValid()) {
                continue;
            }
            insertIfAvailable(child);
            const int childRows = q->sourceModel()->rowCount(child);
            if (childRows > 0) {
                rowsInserted(child, 0, childRows - 1);
            }
        }
    }

    void dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
    {
        for (int row = topLeft.row(); row <= bottomRight.row(); row++) {
            const QModelIndex idx = topLeft.sibling(row, 0);
            insertIfAvailable(idx);
        }
    }

    /**
     *  Selects the index in the internal selection model to make the collection visible in the model
     */
    void select(Collection::Id collectionId)
    {
        const QModelIndex index = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (index.isValid()) {
            q->selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }

    void deselect(Collection::Id collectionId)
    {
        const QModelIndex idx = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (idx.isValid()) {
            q->selectionModel()->select(idx, QItemSelectionModel::Deselect);
        }
    }

    void reference(Collection::Id collectionId)
    {
        if (referencedCollections.contains(collectionId)) {
            qCWarning(AKONADICORE_LOG) << "already referenced " << collectionId;
            return;
        }
        const QModelIndex index = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (index.isValid()) {
            if (q->sourceModel()->setData(index, QVariant(), EntityTreeModel::CollectionRefRole)) {
                referencedCollections << collectionId;
            } else {
                qCWarning(AKONADICORE_LOG) << "failed to reference collection";
            }
            q->sourceModel()->fetchMore(index);
        }
    }

    void dereference(Collection::Id collectionId)
    {
        if (!referencedCollections.contains(collectionId)) {
            qCWarning(AKONADICORE_LOG) << "not referenced " << collectionId;
            return;
        }
        const QModelIndex index = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (index.isValid()) {
            q->sourceModel()->setData(index, QVariant(), EntityTreeModel::CollectionDerefRole);
            referencedCollections.remove(collectionId);
        }
    }

    /**
     * Adds a collection to the favorite collections
     */
    void add(Collection::Id collectionId)
    {
        if (collectionIds.contains(collectionId)) {
            qCDebug(AKONADICORE_LOG) << "already in model " << collectionId;
            return;
        }
        collectionIds << collectionId;
        reference(collectionId);
        select(collectionId);
        const auto idx = EntityTreeModel::modelIndexForCollection(q, Collection{collectionId});
        if (idx.isValid()) {
            auto col = q->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
            if (col.isValid() && !col.hasAttribute<FavoriteCollectionAttribute>()) {
                col.addAttribute(new FavoriteCollectionAttribute());
                new CollectionModifyJob(col, q);
            }
        }
    }

    void remove(Collection::Id collectionId)
    {
        collectionIds.removeAll(collectionId);
        labelMap.remove(collectionId);
        dereference(collectionId);
        deselect(collectionId);
        const auto idx = EntityTreeModel::modelIndexForCollection(q, Collection{collectionId});
        if (idx.isValid()) {
            auto col = q->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
            if (col.isValid() && col.hasAttribute<FavoriteCollectionAttribute>()) {
                col.removeAttribute<FavoriteCollectionAttribute>();
                new CollectionModifyJob(col, q);
            }
        }
    }

    void set(const QList<Collection::Id> &collections)
    {
        QList<Collection::Id> colIds = collectionIds;
        for (const Collection::Id &col : collections) {
            const int removed = colIds.removeAll(col);
            const bool isNewCollection = removed <= 0;
            if (isNewCollection) {
                add(col);
            }
        }
        // Remove what's left
        for (Akonadi::Collection::Id colId : std::as_const(colIds)) {
            remove(colId);
        }
    }

    void set(const Akonadi::Collection::List &collections)
    {
        QList<Akonadi::Collection::Id> colIds;
        colIds.reserve(collections.count());
        for (const Akonadi::Collection &col : collections) {
            colIds << col.id();
        }
        set(colIds);
    }

    void loadConfig()
    {
        const QList<Collection::Id> collections = configGroup.readEntry("FavoriteCollectionIds", QList<qint64>());
        const QStringList labels = configGroup.readEntry("FavoriteCollectionLabels", QStringList());
        const int numberOfLabels(labels.size());
        for (int i = 0; i < collections.size(); ++i) {
            if (i < numberOfLabels) {
                labelMap[collections[i]] = labels[i];
            }
            add(collections[i]);
        }
    }

    void saveConfig()
    {
        QStringList labels;
        labels.reserve(collectionIds.count());
        for (const Collection::Id &collectionId : std::as_const(collectionIds)) {
            labels << labelForCollection(collectionId);
        }

        configGroup.writeEntry("FavoriteCollectionIds", collectionIds);
        configGroup.writeEntry("FavoriteCollectionLabels", labels);
        configGroup.config()->sync();
    }

    FavoriteCollectionsModel *const q;

    QList<Collection::Id> collectionIds;
    QSet<Collection::Id> referencedCollections;
    QHash<qint64, QString> labelMap;
    KConfigGroup configGroup;
};

/* Implementation note:
 *
 * We use KSelectionProxyModel in order to make a flat list of selected folders from the folder tree.
 *
 * Attempts to use QSortFilterProxyModel make code somewhat simpler,
 * but don't work since we then get a filtered tree, not a flat list. Stacking a KDescendantsProxyModel
 * on top would likely remove explicitly selected parents when one of their child is selected too.
 */

FavoriteCollectionsModel::FavoriteCollectionsModel(QAbstractItemModel *source, const KConfigGroup &group, QObject *parent)
    : KSelectionProxyModel(new QItemSelectionModel(source, parent), parent)
    , d(new FavoriteCollectionsModelPrivate(group, this))
{
    setSourceModel(source);
    setFilterBehavior(ExactSelection);

    d->loadConfig();
    // React to various changes in the source model
    connect(source, &QAbstractItemModel::modelReset, this, [this]() {
        d->reload();
    });
    connect(source, &QAbstractItemModel::layoutChanged, this, [this]() {
        d->reload();
    });
    connect(source, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex &parent, int begin, int end) {
        d->rowsInserted(parent, begin, end);
    });
    connect(source, &QAbstractItemModel::dataChanged, this, [this](const QModelIndex &tl, const QModelIndex &br) {
        d->dataChanged(tl, br);
    });
}

FavoriteCollectionsModel::~FavoriteCollectionsModel() = default;

void FavoriteCollectionsModel::setCollections(const Collection::List &collections)
{
    d->set(collections);
    d->saveConfig();
}

void FavoriteCollectionsModel::addCollection(const Collection &collection)
{
    d->add(collection.id());
    d->saveConfig();
}

void FavoriteCollectionsModel::removeCollection(const Collection &collection)
{
    d->remove(collection.id());
    d->saveConfig();
}

Akonadi::Collection::List FavoriteCollectionsModel::collections() const
{
    Collection::List cols;
    cols.reserve(d->collectionIds.count());
    for (const Collection::Id &colId : std::as_const(d->collectionIds)) {
        const QModelIndex idx = EntityTreeModel::modelIndexForCollection(sourceModel(), Collection(colId));
        const auto collection = sourceModel()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
        cols << collection;
    }
    return cols;
}

QList<Collection::Id> FavoriteCollectionsModel::collectionIds() const
{
    return d->collectionIds;
}

void Akonadi::FavoriteCollectionsModel::setFavoriteLabel(const Collection &collection, const QString &label)
{
    Q_ASSERT(d->collectionIds.contains(collection.id()));
    d->labelMap[collection.id()] = label;
    d->saveConfig();

    const QModelIndex idx = EntityTreeModel::modelIndexForCollection(sourceModel(), collection);

    if (!idx.isValid()) {
        return;
    }

    const QModelIndex index = mapFromSource(idx);
    Q_EMIT dataChanged(index, index);
}

QVariant Akonadi::FavoriteCollectionsModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == 0 && (role == Qt::DisplayRole || role == Qt::EditRole)) {
        const QModelIndex sourceIndex = mapToSource(index);
        const Collection::Id collectionId = sourceModel()->data(sourceIndex, EntityTreeModel::CollectionIdRole).toLongLong();

        return d->labelForCollection(collectionId);
    } else {
        return KSelectionProxyModel::data(index, role);
    }
}

bool FavoriteCollectionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && index.column() == 0 && role == Qt::EditRole) {
        const QString newLabel = value.toString();
        if (newLabel.isEmpty()) {
            return false;
        }
        const QModelIndex sourceIndex = mapToSource(index);
        const auto collection = sourceModel()->data(sourceIndex, EntityTreeModel::CollectionRole).value<Collection>();
        setFavoriteLabel(collection, newLabel);
        return true;
    }
    return KSelectionProxyModel::setData(index, value, role);
}

QString Akonadi::FavoriteCollectionsModel::favoriteLabel(const Akonadi::Collection &collection)
{
    if (!collection.isValid()) {
        return QString();
    }
    return d->labelForCollection(collection.id());
}

QString Akonadi::FavoriteCollectionsModel::defaultFavoriteLabel(const Akonadi::Collection &collection)
{
    if (!collection.isValid()) {
        return QString();
    }

    const auto colIdx = EntityTreeModel::modelIndexForCollection(sourceModel(), Collection(collection.id()));
    const QString nameOfCollection = colIdx.data().toString();

    QModelIndex idx = colIdx.parent();
    QString accountName;
    while (idx != QModelIndex()) {
        accountName = idx.data(EntityTreeModel::OriginalCollectionNameRole).toString();
        idx = idx.parent();
    }
    if (accountName.isEmpty()) {
        return nameOfCollection;
    } else {
        return nameOfCollection + QStringLiteral(" (") + accountName + QLatin1Char(')');
    }
}

QVariant FavoriteCollectionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return i18n("Favorite Folders");
    } else {
        return KSelectionProxyModel::headerData(section, orientation, role);
    }
}

bool FavoriteCollectionsModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action)
    Q_UNUSED(row)
    Q_UNUSED(column)
    if (data->hasFormat(QStringLiteral("text/uri-list"))) {
        const QList<QUrl> urls = data->urls();

        const QModelIndex sourceIndex = mapToSource(parent);
        const auto destCollection = sourceModel()->data(sourceIndex, EntityTreeModel::CollectionRole).value<Collection>();

        MimeTypeChecker mimeChecker;
        mimeChecker.setWantedMimeTypes(destCollection.contentMimeTypes());

        for (const QUrl &url : urls) {
            const Collection col = Collection::fromUrl(url);
            if (col.isValid()) {
                addCollection(col);
            } else {
                const Item item = Item::fromUrl(url);
                if (item.isValid()) {
                    if (item.parentCollection().id() == destCollection.id() && action != Qt::CopyAction) {
                        qCDebug(AKONADICORE_LOG) << "Error: source and destination of move are the same.";
                        return false;
                    }
#if 0
                    if (!mimeChecker.isWantedItem(item)) {
                        qCDebug(AKONADICORE_LOG) << "unwanted item" << mimeChecker.wantedMimeTypes() << item.mimeType();
                        return false;
                    }
#endif
                    KJob *job = PasteHelper::pasteUriList(data, destCollection, action);
                    if (!job) {
                        return false;
                    }
                    connect(job, &KJob::result, this, &FavoriteCollectionsModel::pasteJobDone);
                    // Accept the event so that it doesn't propagate.
                    return true;
                }
            }
        }
        return true;
    }
    return false;
}

QStringList FavoriteCollectionsModel::mimeTypes() const
{
    QStringList mts = KSelectionProxyModel::mimeTypes();
    if (!mts.contains(QLatin1StringView("text/uri-list"))) {
        mts.append(QStringLiteral("text/uri-list"));
    }
    return mts;
}

Qt::ItemFlags FavoriteCollectionsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags fs = KSelectionProxyModel::flags(index);
    if (!index.isValid()) {
        fs |= Qt::ItemIsDropEnabled;
    }
    return fs;
}

void FavoriteCollectionsModel::pasteJobDone(KJob *job)
{
    if (job->error()) {
        qCDebug(AKONADICORE_LOG) << "Paste job error:" << job->errorString();
    }
}

#include "moc_favoritecollectionsmodel.cpp"
