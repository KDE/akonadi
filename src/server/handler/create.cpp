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

#include <QtCore/QDebug>
#include <QtCore/QStringList>

#include "connection.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "handlerhelper.h"
#include "storage/selectquerybuilder.h"
#include "response.h"
#include "imapstreamparser.h"

#include <private/protocol_p.h>
#include <private/imapparser_p.h>

using namespace Akonadi;
using namespace Akonadi::Server;

bool Create::parseStream()
{
    Protocol::CreateCollectionCommand cmd;
    mInStream >> cmd;

    if (cmd.name().isEmpty()) {
        return failureResponse<Protocol::CreateCollectionResponse>(QStringLiteral("Invalid collection name"));
    }

    bool ok = false;
    Collection parent = HandlerHelper::collectionFromScope(cmd.parent());
    if (!parent.isValid()) {
        return failureResponse<Protocol::CreateCollectionResponse>(QStringLiteral("Invalid parent collection"));
    }

    qint64 resourceId = 0;
    bool forceVirtual = false;

    // check if parent can contain a sub-folder
    const MimeType::List parentContentTypes = parent.mimeTypes();
    bool found = false, foundVirtual = false;
    for (const MimeType &mt : parentContentTypes) {
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
        return failureResponse<CreateCollectionCommand>(QStringLiteral("Parent collection can not contain sub-collections"));
    }

    // If only virtual collections are supported, force every new collection to
    // be virtual. Otherwise depend on VIRTUAL attribute in the command
    if (foundVirtual && !found) {
        forceVirtual = true;
    }

    // inherit resource
    resourceId = parent.resourceId();

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
    collection.setCachePolicyLocalParts(cp.localParts());
    collection.setCachePolicySyncOnDemand(cp.syncOnDemand());

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db);

    if (!db->appendCollection(collection)) {
        return failureResponse<Protocol::CreateCollectionResponse>(
            QStringLiteral("Could not create collection ") % cmd.name()
                % QStringLiteral(", resourceId: ") % QString::number(resourceId));
    }

    QStringList effectiveMimeTypes = cmd.mimeTypes();
    if (effectiveMimeTypes.isEmpty()) {
        for (const MimeType &mt : parentContentTypes) {
            effectiveMimeTypes << mt.name();
        }
    }
    if (!db->appendMimeTypeForCollection(collection.id(), effectiveMimeTypes)) {
        return failureResponse<Protocol::CreateCollectionResponse>(
            QStringLiteral("Unable to append mimetype for collection ") % cmd.name()
                % QStringLiteral(" resourceId: ") % QString::number(resourceId));
    }

    // store user defined attributes
    for (const QPair<QByteArray, QByteArray> &attr : cmd.attributes()) {
        if (!db->addCollectionAttribute(collection, attr.first, attr.second)) {
            return failureResponse<Protocol::CreateCollectionResponse>(
                QStringLiteral("Unable to add collection attribute."));
        }
    }

    if (!transaction.commit()) {
        return failureResponse<Protocol::CreateCollectionResponse>(
            QStringLiteral("Unable to commit transaction."));
    }

    db->activeCachePolicy(collection);
    mOutStream << HandlerHelper::collectionResponse(collection);
    mOutStream << Protocol::CreateCollectionResponse();
    return true;
}
