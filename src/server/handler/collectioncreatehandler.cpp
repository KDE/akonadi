/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Ingo Kloecker <kloecker@kde.org>         *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/
#include "collectioncreatehandler.h"

#include "connection.h"
#include "handlerhelper.h"
#include "shared/akranges.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include "private/scope_p.h"

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

namespace
{

[[nodiscard]] bool isTopLevelCollection(const Scope &scope)
{
    return scope.scope() == Scope::Uid && scope.uidSet().size() == 1 && scope.uid() == 0;
}

} // namespace

CollectionCreateHandler::CollectionCreateHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool CollectionCreateHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::CreateCollectionCommand>(m_command);

    if (cmd.name().isEmpty()) {
        return failureResponse(QStringLiteral("Invalid collection name"));
    }

    Collection parent;
    qint64 resourceId = 0;
    bool forceVirtual = false;
    MimeType::List parentContentTypes;

    // Invalid or empty scope means we refer to root collection
    if (!isTopLevelCollection(cmd.parent())) {
        parent = HandlerHelper::collectionFromScope(cmd.parent(), connection()->context());
        if (!parent.isValid()) {
            return failureResponse(QStringLiteral("Invalid parent collection"));
        }

        // check if parent can contain a sub-folder
        parentContentTypes = parent.mimeTypes();
        const auto hasMimeType = [](const QString &mimeType) {
            return [mimeType](const MimeType &mt) {
                return mt.name() == mimeType;
            };
        };
        const bool canContainCollections = parentContentTypes | Actions::any(hasMimeType(CollectionMimeType));
        const bool canContainVirtualCollections = parentContentTypes | Actions::any(hasMimeType(VirtualCollectionMimeType));

        if (!canContainCollections && !canContainVirtualCollections) {
            return failureResponse(QStringLiteral("Parent collection can not contain sub-collections"));
        }

        // If only virtual collections are supported, force every new collection to
        // be virtual. Otherwise depend on VIRTUAL attribute in the command
        if (canContainVirtualCollections && !canContainCollections) {
            forceVirtual = true;
        }

        // inherit resource
        resourceId = parent.resourceId();
    } else {
        const QString sessionId = QString::fromUtf8(connection()->sessionId());
        Resource res = Resource::retrieveByName(sessionId);
        if (!res.isValid()) {
            return failureResponse(QStringLiteral("Cannot create top-level collection"));
        }
        resourceId = res.id();
    }

    Collection collection;
    if (parent.isValid()) {
        collection.setParentId(parent.id());
    }
    collection.setName(cmd.name());
    collection.setResourceId(resourceId);
    collection.setRemoteId(cmd.remoteId());
    collection.setRemoteRevision(cmd.remoteRevision());
    collection.setIsVirtual(cmd.isVirtual() || forceVirtual);
    collection.setEnabled(cmd.enabled());
    collection.setSyncPref(static_cast<Collection::Tristate>(cmd.syncPref()));
    collection.setDisplayPref(static_cast<Collection::Tristate>(cmd.displayPref()));
    collection.setIndexPref(static_cast<Collection::Tristate>(cmd.indexPref()));
    const Protocol::CachePolicy &cp = cmd.cachePolicy();
    collection.setCachePolicyCacheTimeout(cp.cacheTimeout());
    collection.setCachePolicyCheckInterval(cp.checkInterval());
    collection.setCachePolicyInherit(cp.inherit());
    collection.setCachePolicyLocalParts(cp.localParts().join(u' '));
    collection.setCachePolicySyncOnDemand(cp.syncOnDemand());

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db, QStringLiteral("CREATE"));

    QStringList effectiveMimeTypes = cmd.mimeTypes();
    if (effectiveMimeTypes.isEmpty()) {
        effectiveMimeTypes = parentContentTypes | Views::transform(&MimeType::name) | Actions::toQList;
    }

    if (!db->appendCollection(collection, effectiveMimeTypes, cmd.attributes())) {
        return failureResponse(QStringLiteral("Could not create collection %1, resourceId %2").arg(cmd.name()).arg(resourceId));
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Unable to commit transaction."));
    }

    db->activeCachePolicy(collection);

    sendResponse<Protocol::FetchCollectionsResponse>(HandlerHelper::fetchCollectionsResponse(akonadi(), collection));

    return successResponse<Protocol::CreateCollectionResponse>();
}
