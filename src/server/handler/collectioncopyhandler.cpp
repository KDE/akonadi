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

#include "collectioncopyhandler.h"

#include "connection.h"
#include "handler/itemcopyhandler.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "protocol_p.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/itemretriever.h"
#include "storage/collectionqueryhelper.h"
#include "shared/akranges.h"

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

CollectionCopyHandler::CollectionCopyHandler(AkonadiServer &akonadi)
    : ItemCopyHandler(akonadi)
{}

bool CollectionCopyHandler::copyCollection(const Collection &source, const Collection &target)
{
    if (!CollectionQueryHelper::canBeMovedTo(source, target)) {
        // We don't accept source==target, or source being an ancestor of target.
        return false;
    }

    // copy the source collection
    Collection col = source;
    col.setParentId(target.id());
    col.setResourceId(target.resourceId());
    // clear remote id and revision on inter-resource copies
    if (source.resourceId() != target.resourceId()) {
        col.setRemoteId(QString());
        col.setRemoteRevision(QString());
    }

    const auto mimeTypes = source.mimeTypes() | Views::transform(&MimeType::name) | Actions::toQList;
    const auto attributes = source.attributes() | Views::transform([](const auto &attr) { return std::make_pair(attr.type(), attr.value()); }) | Actions::toQMap;

    if (!storageBackend()->appendCollection(col, mimeTypes, attributes)) {
        return false;
    }


    // copy sub-collections
    const Collection::List lstCols = source.children();
    for (const Collection &child : lstCols) {
        if (!copyCollection(child, col)) {
            return false;
        }
    }

    // copy items
    const auto items = source.items();
    for (const auto &item : items) {
        if (!copyItem(item, col)) {
            return false;
        }
    }

    return true;
}

bool CollectionCopyHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::CopyCollectionCommand>(m_command);

    const Collection source = HandlerHelper::collectionFromScope(cmd.collection(), connection()->context());
    if (!source.isValid()) {
        return failureResponse(QStringLiteral("No valid source specified"));
    }

    const Collection target = HandlerHelper::collectionFromScope(cmd.destination(), connection()->context());
    if (!target.isValid()) {
        return failureResponse(QStringLiteral("No valid target specified"));
    }

    CacheCleanerInhibitor inhibitor(akonadi());

    // retrieve all not yet cached items of the source
    ItemRetriever retriever(connection(), connection()->context());
    retriever.setCollection(source, true);
    retriever.setRetrieveFullPayload(true);
    if (!retriever.exec()) {
        return failureResponse(retriever.lastError());
    }

    Transaction transaction(storageBackend(), QStringLiteral("CollectionCopyHandler"));

    if (!copyCollection(source, target)) {
        return failureResponse(QStringLiteral("Failed to copy collection"));
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Cannot commit transaction."));
    }

    return successResponse<Protocol::CopyCollectionResponse>();
}
