/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemcopyhandler.h"

#include "akonadi.h"
#include "cachecleaner.h"
#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretriever.h"
#include "storage/parthelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include "private/imapset_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;

ItemCopyHandler::ItemCopyHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool ItemCopyHandler::copyItem(const PimItem &item, const Collection &target)
{
    PimItem newItem = item;
    newItem.setId(-1);
    newItem.setRev(0);
    newItem.setDatetime(QDateTime::currentDateTimeUtc());
    newItem.setAtime(QDateTime::currentDateTimeUtc());
    newItem.setRemoteId(QString());
    newItem.setRemoteRevision(QString());
    newItem.setCollectionId(target.id());
    Part::List newParts;
    const auto parts = item.parts();
    newParts.reserve(parts.size());
    for (const Part &part : parts) {
        Part newPart(part);
        newPart.setData(PartHelper::translateData(newPart.data(), part.storage()));
        newPart.setPimItemId(-1);
        newPart.setStorage(Part::Internal);
        newParts << newPart;
    }

    DataStore *store = connection()->storageBackend();
    return store->appendPimItem(newParts, item.flags(), item.mimeType(), target, QDateTime::currentDateTimeUtc(), QString(), QString(), item.gid(), newItem);
}

void ItemCopyHandler::processItems(const QList<qint64> &ids)
{
    SelectQueryBuilder<PimItem> qb;
    ItemQueryHelper::itemSetToQuery(ImapSet(ids), qb);
    if (!qb.exec()) {
        failureResponse(QStringLiteral("Unable to retrieve items"));
        return;
    }
    const PimItem::List items = qb.result();
    qb.query().finish();

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store, QStringLiteral("COPY"));

    for (const PimItem &item : items) {
        if (!copyItem(item, mTargetCollection)) {
            failureResponse(QStringLiteral("Unable to copy item"));
            return;
        }
    }

    if (!transaction.commit()) {
        failureResponse(QStringLiteral("Cannot commit transaction."));
        return;
    }
}

bool ItemCopyHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::CopyItemsCommand>(m_command);

    if (!checkScopeConstraints(cmd.items(), Scope::Uid)) {
        return failureResponse(QStringLiteral("Only UID copy is allowed"));
    }

    if (cmd.items().isEmpty()) {
        return failureResponse(QStringLiteral("No items specified"));
    }

    mTargetCollection = HandlerHelper::collectionFromScope(cmd.destination(), connection()->context());
    if (!mTargetCollection.isValid()) {
        return failureResponse(QStringLiteral("No valid target specified"));
    }
    if (mTargetCollection.isVirtual()) {
        return failureResponse(QStringLiteral("Copying items into virtual collections is not allowed"));
    }

    CacheCleanerInhibitor inhibitor(akonadi());

    ItemRetriever retriever(akonadi().itemRetrievalManager(), connection(), connection()->context());
    retriever.setItemSet(cmd.items().uidSet());
    retriever.setRetrieveFullPayload(true);
    QObject::connect(&retriever, &ItemRetriever::itemsRetrieved, &retriever, [this](const QList<qint64> &ids) {
        processItems(ids);
    });
    if (!retriever.exec()) {
        return failureResponse(retriever.lastError());
    }

    return successResponse<Protocol::CopyItemsResponse>();
}
