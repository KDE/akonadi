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

#include "favoritecollectionsmodel.h"

#include <QItemSelectionModel>
#include <QtCore/QMimeData>

#include <kconfiggroup.h>
#include <klocalizedstring.h>
#include <KJob>
#include <QUrl>
#include <KConfig>

#include "entitytreemodel.h"
#include "mimetypechecker.h"
#include "pastehelper_p.h"

using namespace Akonadi;

/**
 * @internal
 */
class FavoriteCollectionsModel::Private
{
public:
    Private(const KConfigGroup &group, FavoriteCollectionsModel *parent)
        : q(parent)
        , configGroup(group)
    {
    }

    QString labelForCollection(Collection::Id collectionId) const
    {
        if (labelMap.contains(collectionId)) {
            return labelMap[collectionId];
        }

        const QModelIndex collectionIdx = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));

        QString accountName;

        const QString nameOfCollection = collectionIdx.data().toString();

        QModelIndex idx = collectionIdx.parent();
        while (idx != QModelIndex()) {
            accountName = idx.data().toString();
            idx = idx.parent();
        }

        if (accountName.isEmpty()) {
            return nameOfCollection;
        } else {
            return nameOfCollection + QStringLiteral(" (") + accountName + QLatin1Char(')');
        }
    }

    void insertIfAvailable(Collection::Id col)
    {
        if (collectionIds.contains(col)) {
            select(col);
            if (!referencedCollections.contains(col)) {
                reference(col);
            }
        }
    }

    void insertIfAvailable(const QModelIndex &idx)
    {
        insertIfAvailable(idx.data(EntityTreeModel::CollectionIdRole).value<Collection::Id>());
    }

    /**
     * Stuff changed, reload everything.
     */
    void reload()
    {
        //don't clear the selection model here. Otherwise we mess up the users selection as collections get removed and re-inserted.
        foreach (const Collection::Id &collectionId, collectionIds) {
            insertIfAvailable(collectionId);
        }
        //TODO remove what's no longer here
    }

    void rowsInserted(const QModelIndex &parent, int begin, int end)
    {
        for (int row = begin; row <= end; row++) {
            const QModelIndex child = parent.child(row, 0);
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
            const QModelIndex idx = topLeft.parent().child(row, 0);
            insertIfAvailable(idx);
        }
    }

    /**
     *  Selects the index in the internal selection model to make the collection visible in the model
     */
    void select(const Collection::Id &collectionId)
    {
        const QModelIndex index = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (index.isValid()) {
            q->selectionModel()->select(index, QItemSelectionModel::Select);
        }
    }

    void deselect(const Collection::Id &collectionId)
    {
        const QModelIndex idx = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (idx.isValid()) {
            q->selectionModel()->select(idx, QItemSelectionModel::Deselect);
        }
    }

    void reference(const Collection::Id &collectionId)
    {
        if (referencedCollections.contains(collectionId)) {
            qWarning() << "already referenced " << collectionId;
            return;
        }
        const QModelIndex index = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (index.isValid()) {
            if (q->sourceModel()->setData(index, QVariant(), EntityTreeModel::CollectionRefRole)) {
                referencedCollections << collectionId;
            } else {
                qWarning() << "failed to reference collection";
            }
            q->sourceModel()->fetchMore(index);
        }
    }

    void dereference(const Collection::Id &collectionId)
    {
        if (!referencedCollections.contains(collectionId)) {
            qWarning() << "not referenced " << collectionId;
            return;
        }
        const QModelIndex index = EntityTreeModel::modelIndexForCollection(q->sourceModel(), Collection(collectionId));
        if (index.isValid()) {
            q->sourceModel()->setData(index, QVariant(), EntityTreeModel::CollectionDerefRole);
        }
	referencedCollections.remove(collectionId);
    }

    void clearReferences()
    {
        foreach (const Collection::Id &collectionId, referencedCollections) {
            dereference(collectionId);
        }
    }

    /**
     * Adds a collection to the favorite collections
     */
    void add(const Collection::Id &collectionId)
    {
        if (collectionIds.contains(collectionId)) {
            qDebug() << "already in model " << collectionId;
            return;
        }
        collectionIds << collectionId;
        reference(collectionId);
        select(collectionId);
    }

    void remove(const Collection::Id &collectionId)
    {
        collectionIds.removeAll(collectionId);
        labelMap.remove(collectionId);
        dereference(collectionId);
        deselect(collectionId);
    }

    void set(const QList<Collection::Id> &collections)
    {
        QList<Collection::Id> colIds = collectionIds;
        foreach (const Collection::Id &col, collections) {
            const int removed = colIds.removeAll(col);
            const bool isNewCollection = removed <= 0;
            if (isNewCollection) {
                add(col);
            }
        }
        //Remove what's left
        foreach (const Akonadi::Collection::Id &colId, colIds) {
            remove(colId);
        }
    }

    void set(const Akonadi::Collection::List &collections)
    {
        QList<Akonadi::Collection::Id> colIds;
        foreach (const Akonadi::Collection &col, collections) {
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

        foreach (const Collection::Id &collectionId, collectionIds) {
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

FavoriteCollectionsModel::FavoriteCollectionsModel(QAbstractItemModel *source, const KConfigGroup &group, QObject *parent)
    : Akonadi::SelectionProxyModel(new QItemSelectionModel(source, parent), parent)
    , d(new Private(group, this))
{
    //This should only be a KRecursiveFilterProxyModel, but remains a SelectionProxyModel for backwards compatiblity.
    // We therefore disable what we anyways don't want (the referencing is handled separately).
    disconnect(this, SIGNAL(rootIndexAdded(QModelIndex)), this, SLOT(rootIndexAdded(QModelIndex)));
    disconnect(this, SIGNAL(rootIndexAboutToBeRemoved(QModelIndex)), this, SLOT(rootIndexAboutToBeRemoved(QModelIndex)));

    setSourceModel(source);
    setFilterBehavior(ExactSelection);

    d->loadConfig();
    //React to various changes in the source model
    connect(source, SIGNAL(modelReset()), this, SLOT(reload()));
    connect(source, SIGNAL(layoutChanged()), this, SLOT(reload()));
    connect(source, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(rowsInserted(QModelIndex,int,int)));
    connect(source, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(dataChanged(QModelIndex,QModelIndex)));
}

FavoriteCollectionsModel::~FavoriteCollectionsModel()
{
    delete d;
}

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
    foreach (const Collection::Id &colId, d->collectionIds) {
        const QModelIndex idx = EntityTreeModel::modelIndexForCollection(sourceModel(), Collection(colId));
        const Collection collection = sourceModel()->data(idx, EntityTreeModel::CollectionRole).value<Collection>();
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
    emit dataChanged(index, index);
}

QVariant Akonadi::FavoriteCollectionsModel::data(const QModelIndex &index, int role) const
{
    if (index.column() == 0 &&
        (role == Qt::DisplayRole ||
         role == Qt::EditRole)) {
        const QModelIndex sourceIndex = mapToSource(index);
        const Collection::Id collectionId = sourceModel()->data(sourceIndex, EntityTreeModel::CollectionIdRole).toLongLong();

        return d->labelForCollection(collectionId);
    } else {
        return KSelectionProxyModel::data(index, role);
    }
}

bool FavoriteCollectionsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && index.column() == 0 &&
        role == Qt::EditRole) {
        const QString newLabel = value.toString();
        if (newLabel.isEmpty()) {
            return false;
        }
        const QModelIndex sourceIndex = mapToSource(index);
        const Collection collection = sourceModel()->data(sourceIndex, EntityTreeModel::CollectionRole).value<Collection>();
        setFavoriteLabel(collection, newLabel);
        return true;
    }
    return Akonadi::SelectionProxyModel::setData(index, value, role);
}

QString Akonadi::FavoriteCollectionsModel::favoriteLabel(const Akonadi::Collection &collection)
{
    if (!collection.isValid()) {
        return QString();
    }
    return d->labelForCollection(collection.id());
}

QVariant FavoriteCollectionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (section == 0 &&
        orientation == Qt::Horizontal &&
        role == Qt::DisplayRole) {
        return i18n("Favorite Folders");
    } else {
        return KSelectionProxyModel::headerData(section, orientation, role);
    }
}

bool FavoriteCollectionsModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(action);
    Q_UNUSED(row);
    Q_UNUSED(column);
    if (data->hasFormat(QStringLiteral("text/uri-list"))) {
        const QList<QUrl> urls = data->urls();

        const QModelIndex sourceIndex = mapToSource(parent);
        const Collection destCollection = sourceModel()->data(sourceIndex, EntityTreeModel::CollectionRole).value<Collection>();

        MimeTypeChecker mimeChecker;
        mimeChecker.setWantedMimeTypes(destCollection.contentMimeTypes());

        foreach (const QUrl &url, urls) {
            const Collection col = Collection::fromUrl(url);
            if (col.isValid()) {
                addCollection(col);
            } else {
                const Item item = Item::fromUrl(url);
                if (item.isValid()) {
                    if (item.parentCollection().id() == destCollection.id() &&
                        action != Qt::CopyAction) {
                        qDebug() << "Error: source and destination of move are the same.";
                        return false;
                    }
#if 0
                    if (!mimeChecker.isWantedItem(item)) {
                        qDebug() << "unwanted item" << mimeChecker.wantedMimeTypes() << item.mimeType();
                        return false;
                    }
#endif
                    KJob *job = PasteHelper::pasteUriList(data, destCollection, action);
                    if (!job) {
                        return false;
                    }
                    connect(job, SIGNAL(result(KJob*)), SLOT(pasteJobDone(KJob*)));
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
    QStringList mts = Akonadi::SelectionProxyModel::mimeTypes();
    if (!mts.contains(QStringLiteral("text/uri-list"))) {
        mts.append(QStringLiteral("text/uri-list"));
    }
    return mts;
}

Qt::ItemFlags FavoriteCollectionsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags fs = Akonadi::SelectionProxyModel::flags(index);
    if (!index.isValid()) {
        fs |= Qt::ItemIsDropEnabled;
    }
    return fs;
}

void FavoriteCollectionsModel::pasteJobDone(KJob *job)
{
    if (job->error()) {
        qDebug() << job->errorString();
    }
}

#include "moc_favoritecollectionsmodel.cpp"
