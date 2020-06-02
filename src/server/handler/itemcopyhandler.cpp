/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "itemcopyhandler.h"

#include "akonadi.h"
#include "connection.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretriever.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"
#include "storage/parthelper.h"

#include <private/imapset_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

ItemCopyHandler::ItemCopyHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{}

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
    Part::List parts;
    parts.reserve(item.parts().count());
    Q_FOREACH (const Part &part, item.parts()) {
        Part newPart(part);
        newPart.setData(PartHelper::translateData(newPart.data(), part.storage()));
        newPart.setPimItemId(-1);
        newPart.setStorage(Part::Internal);
        parts << newPart;
    }

    DataStore *store = connection()->storageBackend();
    return store->appendPimItem(parts, item.flags(), item.mimeType(), target, QDateTime::currentDateTimeUtc(), QString(), QString(), item.gid(), newItem);
}

void ItemCopyHandler::processItems(const QVector<qint64> &ids)
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
    QObject::connect(&retriever, &ItemRetriever::itemsRetrieved,
                     [this](const QVector<qint64> &ids) {
                        processItems(ids);
                     });
    if (!retriever.exec()) {
        return failureResponse(retriever.lastError());
    }

    return successResponse<Protocol::CopyItemsResponse>();
}
