/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pastehelper_p.h"

#include "collectioncopyjob.h"
#include "collectionfetchjob.h"
#include "collectionmovejob.h"
#include "item.h"
#include "itemcopyjob.h"
#include "itemcreatejob.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "linkjob.h"
#include "session.h"
#include "transactionsequence.h"
#include "unlinkjob.h"

#include "akonadicore_debug.h"

#include <QUrl>
#include <QUrlQuery>

#include <QByteArray>
#include <QMimeData>

#include <functional>

using namespace Akonadi;

class PasteHelperJob : public Akonadi::TransactionSequence
{
    Q_OBJECT

public:
    explicit PasteHelperJob(Qt::DropAction action,
                            const Akonadi::Item::List &items,
                            const Akonadi::Collection::List &collections,
                            const Akonadi::Collection &destination,
                            QObject *parent = nullptr);
    ~PasteHelperJob() override;

private Q_SLOTS:
    void onDragSourceCollectionFetched(KJob *job);

private:
    void runActions();
    void runItemsActions();
    void runCollectionsActions();

private:
    Akonadi::Item::List mItems;
    Akonadi::Collection::List mCollections;
    Akonadi::Collection mDestCollection;
    Qt::DropAction mAction;
};

PasteHelperJob::PasteHelperJob(Qt::DropAction action,
                               const Item::List &items,
                               const Collection::List &collections,
                               const Collection &destination,
                               QObject *parent)
    : TransactionSequence(parent)
    , mItems(items)
    , mCollections(collections)
    , mDestCollection(destination)
    , mAction(action)
{
    // FIXME: The below code disables transactions in order to avoid data loss due to nested
    // transactions (copy and colcopy in the server doesn't see the items retrieved into the cache and copies empty payloads).
    // Remove once this is fixed properly, see the other FIXME comments.
    setProperty("transactionsDisabled", true);

    Collection dragSourceCollection;
    if (!items.isEmpty() && items.first().parentCollection().isValid()) {
        // Check if all items have the same parent collection ID
        const Collection parent = items.first().parentCollection();
        if (!std::any_of(items.cbegin(), items.cend(), [parent](const Item &item) {
                return item.parentCollection() != parent;
            })) {
            dragSourceCollection = parent;
        }
    }

    if (dragSourceCollection.isValid()) {
        // Disable autocommitting, because starting a Link/Unlink/Copy/Move job
        // after the transaction has ended leaves the job hanging
        setAutomaticCommittingEnabled(false);

        auto fetch = new CollectionFetchJob(dragSourceCollection, CollectionFetchJob::Base, this);
        QObject::connect(fetch, &KJob::finished, this, &PasteHelperJob::onDragSourceCollectionFetched);
    } else {
        runActions();
    }
}

PasteHelperJob::~PasteHelperJob()
{
}

void PasteHelperJob::onDragSourceCollectionFetched(KJob *job)
{
    auto fetch = qobject_cast<CollectionFetchJob *>(job);
    qCDebug(AKONADICORE_LOG) << fetch->error() << fetch->collections().count();
    if (fetch->error() || fetch->collections().count() != 1) {
        runActions();
        commit();
        return;
    }

    // If the source collection is virtual, treat copy and move actions differently
    const Collection sourceCollection = fetch->collections().at(0);
    qCDebug(AKONADICORE_LOG) << "FROM: " << sourceCollection.id() << sourceCollection.name() << sourceCollection.isVirtual();
    qCDebug(AKONADICORE_LOG) << "DEST: " << mDestCollection.id() << mDestCollection.name() << mDestCollection.isVirtual();
    qCDebug(AKONADICORE_LOG) << "ACTN:" << mAction;
    if (sourceCollection.isVirtual()) {
        switch (mAction) {
        case Qt::CopyAction:
            if (mDestCollection.isVirtual()) {
                new LinkJob(mDestCollection, mItems, this);
            } else {
                new ItemCopyJob(mItems, mDestCollection, this);
            }
            break;
        case Qt::MoveAction:
            new UnlinkJob(sourceCollection, mItems, this);
            if (mDestCollection.isVirtual()) {
                new LinkJob(mDestCollection, mItems, this);
            } else {
                new ItemCopyJob(mItems, mDestCollection, this);
            }
            break;
        case Qt::LinkAction:
            new LinkJob(mDestCollection, mItems, this);
            break;
        default:
            Q_ASSERT(false);
        }
        runCollectionsActions();
        commit();
    } else {
        runActions();
    }

    commit();
}

void PasteHelperJob::runActions()
{
    runItemsActions();
    runCollectionsActions();
}

void PasteHelperJob::runItemsActions()
{
    if (mItems.isEmpty()) {
        return;
    }

    switch (mAction) {
    case Qt::CopyAction:
        new ItemCopyJob(mItems, mDestCollection, this);
        break;
    case Qt::MoveAction:
        new ItemMoveJob(mItems, mDestCollection, this);
        break;
    case Qt::LinkAction:
        new LinkJob(mDestCollection, mItems, this);
        break;
    default:
        Q_ASSERT(false); // WTF?!
    }
}

void PasteHelperJob::runCollectionsActions()
{
    if (mCollections.isEmpty()) {
        return;
    }

    switch (mAction) {
    case Qt::CopyAction:
        for (const Collection &col : std::as_const(mCollections)) { // FIXME: remove once we have a batch job for collections as well
            new CollectionCopyJob(col, mDestCollection, this);
        }
        break;
    case Qt::MoveAction:
        for (const Collection &col : std::as_const(mCollections)) { // FIXME: remove once we have a batch job for collections as well
            new CollectionMoveJob(col, mDestCollection, this);
        }
        break;
    case Qt::LinkAction:
        // Not supported for collections
        break;
    default:
        Q_ASSERT(false); // WTF?!
    }
}

bool PasteHelper::canPaste(const QList<QUrl> &clipboardUrls, const Collection &collection, Qt::DropAction action)
{
    if (clipboardUrls.empty() || !collection.isValid()) {
        return false;
    }

    // check that the target collection has the rights to
    // create the pasted items resp. collections
    Collection::Rights neededRights = Collection::ReadOnly;

    for (const QUrl &url : clipboardUrls) {
        const QUrlQuery query(url);
        if (query.hasQueryItem(QStringLiteral("item"))) {
            if (action == Qt::LinkAction) {
                neededRights |= Collection::CanLinkItem;
            } else {
                neededRights |= Collection::CanCreateItem;
            }
        } else if (query.hasQueryItem(QStringLiteral("collection"))) {
            neededRights |= Collection::CanCreateCollection;
        }
    }

    if ((collection.rights() & neededRights) == 0) {
        return false;
    }

    // check that the target collection supports the mime types of the
    // items/collections that shall be pasted
    bool supportsMimeTypes = true;
    for (const QUrl &url : std::as_const(clipboardUrls)) {
        const QUrlQuery query(url);
        // collections do not provide mimetype information, so ignore this check
        if (query.hasQueryItem(QStringLiteral("collection"))) {
            continue;
        }

        const QString mimeType = query.queryItemValue(QStringLiteral("type"));
        if (!collection.contentMimeTypes().contains(mimeType)) {
            supportsMimeTypes = false;
            break;
        }
    }

    return supportsMimeTypes;
}

bool PasteHelper::canPaste(const QMimeData *mimeData, const Collection &collection, Qt::DropAction action)
{
    if (!mimeData || !collection.isValid()) {
        return false;
    }

    if (mimeData->hasUrls()) {
        return canPaste(mimeData->urls(), collection, action);
    }

    return false;
}

KJob *PasteHelper::paste(const QMimeData *mimeData, const Collection &collection, Qt::DropAction action, Session *session)
{
    if (!canPaste(mimeData, collection, action)) {
        return nullptr;
    }

    // we try to drop data not coming with the akonadi:// url
    // find a type the target collection supports
    const QStringList lstFormats = mimeData->formats();
    for (const QString &type : lstFormats) {
        if (!collection.contentMimeTypes().contains(type)) {
            continue;
        }

        QByteArray item = mimeData->data(type);
        // HACK for some unknown reason the data is sometimes 0-terminated...
        if (!item.isEmpty() && item.at(item.size() - 1) == 0) {
            item.resize(item.size() - 1);
        }

        Item it;
        it.setMimeType(type);
        it.setPayloadFromData(item);

        auto job = new ItemCreateJob(it, collection);
        return job;
    }

    if (!mimeData->hasUrls()) {
        return nullptr;
    }

    // data contains an url list
    return pasteUriList(mimeData, collection, action, session);
}

KJob *PasteHelper::pasteUriList(const QMimeData *mimeData, const Collection &destination, Qt::DropAction action, Session *session)
{
    if (!mimeData->hasUrls()) {
        return nullptr;
    }

    if (!canPaste(mimeData, destination, action)) {
        return nullptr;
    }

    const QList<QUrl> urls = mimeData->urls();
    Collection::List collections;
    Item::List items;
    for (const QUrl &url : urls) {
        const QUrlQuery query(url);
        const Collection collection = Collection::fromUrl(url);
        if (collection.isValid()) {
            collections.append(collection);
        }
        Item item = Item::fromUrl(url);
        if (query.hasQueryItem(QStringLiteral("parent"))) {
            item.setParentCollection(Collection(query.queryItemValue(QStringLiteral("parent")).toLongLong()));
        }
        if (item.isValid()) {
            items.append(item);
        }
        // TODO: handle non Akonadi URLs?
    }

    auto job = new PasteHelperJob(action, items, collections, destination, session);

    return job;
}

#include "pastehelper.moc"
