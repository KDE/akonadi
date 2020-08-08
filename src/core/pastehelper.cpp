/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pastehelper_p.h"

#include "interface.h"
#include "collectioncopyjob.h"
#include "collectionmovejob.h"
#include "collectionfetchjob.h"
#include "item.h"
#include "itemcreatejob.h"
#include "itemcopyjob.h"
#include "itemmodifyjob.h"
#include "itemmovejob.h"
#include "linkjob.h"
#include "transactionsequence.h"
#include "session.h"
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
    explicit PasteHelperJob(Qt::DropAction action, const Akonadi::Item::List &items,
                            const Akonadi::Collection::List &collections,
                            const Akonadi::Collection &destination,
                            QObject *parent = nullptr);
    virtual ~PasteHelperJob();

private:
    Task<void> runActions(const Collection &sourceCollection);
    Task<void> runItemsActions(const Collection &sourceCollection);
    Task<void> runCollectionsActions();

private:
    Akonadi::Item::List mItems;
    Akonadi::Collection::List mCollections;
    Akonadi::Collection mDestCollection;
    Qt::DropAction mAction;
};

PasteHelperJob::PasteHelperJob(Qt::DropAction action, const Item::List &items,
                               const Collection::List &collections,
                               const Collection &destination,
                               QObject *parent)
    : TransactionSequence(parent)
    , mItems(items)
    , mCollections(collections)
    , mDestCollection(destination)
    , mAction(action)
{
    //FIXME: The below code disables transactions in order to avoid data loss due to nested
    //transactions (copy and colcopy in the server doesn't see the items retrieved into the cache and copies empty payloads).
    //Remove once this is fixed properly, see the other FIXME comments.
    setProperty("transactionsDisabled", true);

    Collection dragSourceCollection;
    if (!items.isEmpty() && items.first().parentCollection().isValid()) {
        // Check if all items have the same parent collection ID
        const Collection parent = items.first().parentCollection();
        if (!std::any_of(items.cbegin(), items.cend(),
                        [parent](const Item &item) {
                            return item.parentCollection() != parent;
                        })) {
            dragSourceCollection = parent;
        }
    }

    Task<void> task;
    if (dragSourceCollection.isValid()) {
        // Disable autocommitting, because starting a Link/Unlink/Copy/Move job
        // after the transaction has ended leaves the job hanging
        setAutomaticCommittingEnabled(false);

        task = Akonadi::fetchCollection(dragSourceCollection).then(
                [this](const Collection &sourceCollection) {
                    return runActions(sourceCollection);
                },
                [this](const Akonadi::Error &error) {
                    qCWarning(AKONADICORE_LOG) << error;
                    rollback();
                });
    } else {
        task = runActions({});
    }


    task.then([this]() {
        commit();
        emitResult();
    }, [this](const Akonadi::Error &error) {
        qCWarning(AKONADICORE_LOG) << error;
        setError(error.code());
        setErrorText(error.message());
        rollback();
    });
}

PasteHelperJob::~PasteHelperJob() = default;

Task<void> PasteHelperJob::runActions(const Collection &sourceCollection)
{
    return runItemsActions(sourceCollection).then([this]() { return runCollectionsActions(); });
}

Task<void> PasteHelperJob::runItemsActions(const Collection &sourceCollection)
{
    if (mItems.isEmpty()) {
        return Akonadi::finishedTask();
    }

    switch (mAction) {
    case Qt::CopyAction:
        if (mDestCollection.isVirtual()) {
            return Akonadi::linkItems(mItems, mDestCollection);
        } else {
            return Akonadi::copyItems(mItems, mDestCollection);
        }
    case Qt::MoveAction: {
        Task<void> task = mDestCollection.isVirtual()
            ? Akonadi::linkItems(mItems, mDestCollection)
            : Akonadi::moveItems(mItems, mDestCollection);
        if (sourceCollection.isVirtual()) {
            return task.then([this, sourceCollection]() { return Akonadi::unlinkItems(mItems, sourceCollection); });
        }
        return task;
    }
    case Qt::LinkAction:
        return Akonadi::linkItems(mItems, mDestCollection);
    default:
        Q_ASSERT(false); // WTF?!
    }
}

Task<void> PasteHelperJob::runCollectionsActions()
{
    if (mCollections.isEmpty()) {
        return Akonadi::finishedTask();
    }

    switch (mAction) {
    case Qt::CopyAction:
        // FIXME: remove once we have a batch job for collections as well
        return Akonadi::taskForEach(mCollections,
            [dest = mDestCollection](const Collection &col) {
                return Akonadi::copyCollection(col, dest);
            });
    case Qt::MoveAction:
        // FIXME: remove once we have a batch job for collections as well
        return Akonadi::taskForEach(mCollections,
            [dest = mDestCollection](const Collection &col) {
                return Akonadi::moveCollection(col, dest);
            });
    case Qt::LinkAction:
        return Akonadi::finishedTask();
    default:
        Q_ASSERT(false); // WTF?!
    }
}

bool PasteHelper::canPaste(const QMimeData *mimeData, const Collection &collection, Qt::DropAction action)
{
    if (!mimeData || !collection.isValid()) {
        return false;
    }

    // check that the target collection has the rights to
    // create the pasted items resp. collections
    Collection::Rights neededRights = Collection::ReadOnly;
    if (mimeData->hasUrls()) {
        const QList<QUrl> urls = mimeData->urls();
        for (const QUrl &url : urls) {
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
        for (const QUrl &url : qAsConst(urls)) {
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

    return false;
}

KJob *PasteHelper::pasteUriList(const QMimeData *mimeData, const Collection &destination, Qt::DropAction action, Session *session)
{
    if (!mimeData->hasUrls()) {
        return nullptr;
    }

    if (!canPaste(mimeData, destination, action)) {
        qCDebug(AKONADICORE_LOG) << "Cannot paste uris" << mimeData->urls() << "to collection" << destination.id() << ", action=" << action;
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

    return new PasteHelperJob(action, items, collections, destination, session);
}

#include "pastehelper.moc"
