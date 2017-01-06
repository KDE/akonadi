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
#include "create.h"

#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"
#include "utils.h"

#include <private/scope_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Create::parseStream()
{
    Protocol::CreateCollectionCommand cmd(m_command);

    if (cmd.name().isEmpty()) {
        return failureResponse("Invalid collection name");
    }

    Collection parent;
    qint64 resourceId = 0;
    bool forceVirtual = false;
    MimeType::List parentContentTypes;

    // Invalid or empty scope means we refer to root collection
    if (cmd.parent().scope() != Scope::Invalid && !cmd.parent().isEmpty()) {
        parent = HandlerHelper::collectionFromScope(cmd.parent(), connection());
        if (!parent.isValid()) {
            return failureResponse("Invalid parent collection");
        }

        // check if parent can contain a sub-folder
        parentContentTypes = parent.mimeTypes();
        bool found = false, foundVirtual = false;
        for (const MimeType &mt : qAsConst(parentContentTypes)) {
            if (mt.name() == QLatin1String("inode/directory")) {
                found = true;
            } else if (mt.name() == QLatin1String("application/x-vnd.akonadi.collection.virtual")) {
                foundVirtual = true;
            }
            if (found && foundVirtual) {
                break;
            }
        }
        if (!found && !foundVirtual) {
            return failureResponse("Parent collection can not contain sub-collections");
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
            return failureResponse("Cannot create top-level collection");
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
    collection.setSyncPref(cmd.syncPref());
    collection.setDisplayPref(cmd.displayPref());
    collection.setIndexPref(cmd.indexPref());
    const Protocol::CachePolicy &cp = cmd.cachePolicy();
    collection.setCachePolicyCacheTimeout(cp.cacheTimeout());
    collection.setCachePolicyCheckInterval(cp.checkInterval());
    collection.setCachePolicyInherit(cp.inherit());
    collection.setCachePolicyLocalParts(cp.localParts().join(QLatin1Char(' ')));
    collection.setCachePolicySyncOnDemand(cp.syncOnDemand());

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db);

    if (!db->appendCollection(collection)) {
        return failureResponse(QStringLiteral("Could not create collection ") % cmd.name()
                               % QStringLiteral(", resourceId: ") % QString::number(resourceId));
    }

    QStringList effectiveMimeTypes = cmd.mimeTypes();
    if (effectiveMimeTypes.isEmpty()) {
        effectiveMimeTypes.reserve(parentContentTypes.count());
        for (const MimeType &mt : qAsConst(parentContentTypes)) {
            effectiveMimeTypes << mt.name();
        }
    }
    if (!db->appendMimeTypeForCollection(collection.id(), effectiveMimeTypes)) {
        return failureResponse(QStringLiteral("Unable to append mimetype for collection ") % cmd.name()
                               % QStringLiteral(" resourceId: ") % QString::number(resourceId));
    }

    // store user defined attributes
    const QMap<QByteArray, QByteArray> attrs = cmd.attributes();
    for (auto iter = attrs.constBegin(), end = attrs.constEnd(); iter != end; ++iter) {
        if (!db->addCollectionAttribute(collection, iter.key(), iter.value())) {
            return failureResponse("Unable to add collection attribute.");
        }
    }

    if (!transaction.commit()) {
        return failureResponse("Unable to commit transaction.");
    }

    db->activeCachePolicy(collection);

    sendResponse<Protocol::FetchCollectionsResponse>(
        HandlerHelper::fetchCollectionsResponse(collection));

    return successResponse<Protocol::CreateCollectionResponse>();
}
