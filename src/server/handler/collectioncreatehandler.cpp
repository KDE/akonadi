/***************************************************************************
 *   Copyright (C) 2006 by Ingo Kloecker <kloecker@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include "collectioncreatehandler.h"

#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"


#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

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
    if (cmd.parent().scope() != Scope::Invalid && !cmd.parent().isEmpty()) {
        parent = HandlerHelper::collectionFromScope(cmd.parent(), connection());
        if (!parent.isValid()) {
            return failureResponse(QStringLiteral("Invalid parent collection"));
        }

        // check if parent can contain a sub-folder
        parentContentTypes = parent.mimeTypes();
        bool found = false, foundVirtual = false;
        for (const MimeType &mt : qAsConst(parentContentTypes)) {
            const QString mtName{mt.name()};
            if (mtName == QLatin1String("inode/directory")) {
                found = true;
            } else if (mtName == QLatin1String("application/x-vnd.akonadi.collection.virtual")) {
                foundVirtual = true;
            }
            if (found && foundVirtual) {
                break;
            }
        }
        if (!found && !foundVirtual) {
            return failureResponse(QStringLiteral("Parent collection can not contain sub-collections"));
        }

        // If only virtual collections are supported, force every new collection to
        // be virtual. Otherwise depend on VIRTUAL attribute in the command
        if (foundVirtual && !found) {
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
    collection.setCachePolicyLocalParts(cp.localParts().join(QLatin1Char(' ')));
    collection.setCachePolicySyncOnDemand(cp.syncOnDemand());

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db, QStringLiteral("CREATE"));

    QStringList effectiveMimeTypes = cmd.mimeTypes();
    if (effectiveMimeTypes.isEmpty()) {
        effectiveMimeTypes.reserve(parentContentTypes.count());
        std::transform(parentContentTypes.cbegin(), parentContentTypes.cend(),
                       std::back_inserter(effectiveMimeTypes),
                       std::bind(&MimeType::name, std::placeholders::_1));
    }

    if (!db->appendCollection(collection, effectiveMimeTypes, cmd.attributes())) {
        return failureResponse(QStringLiteral("Could not create collection ") % cmd.name()
                               % QStringLiteral(", resourceId: ") % QString::number(resourceId));
    }

    if (!transaction.commit()) {
        return failureResponse(QStringLiteral("Unable to commit transaction."));
    }

    db->activeCachePolicy(collection);

    sendResponse<Protocol::FetchCollectionsResponse>(
        HandlerHelper::fetchCollectionsResponse(collection));

    return successResponse<Protocol::CreateCollectionResponse>();
}
