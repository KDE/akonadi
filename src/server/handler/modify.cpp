/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "modify.h"

#include "akonadi.h"
#include "connection.h"
#include "handlerhelper.h"
#include "cachecleaner.h"
#include "collectionreferencemanager.h"
#include "intervalcheck.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/itemretriever.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionqueryhelper.h"
#include "search/searchmanager.h"

using namespace Akonadi;
using namespace Akonadi::Server;

bool Modify::parseStream()
{
    Protocol::ModifyCollectionCommand cmd(m_command);

    Collection collection = HandlerHelper::collectionFromScope(cmd.collection(), connection());
    if (!collection.isValid()) {
        return failureResponse("No such collection");
    }

    CacheCleanerInhibitor inhibitor(false);

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::ParentID) {
        const Collection newParent = Collection::retrieveById(cmd.parentId());
        if (newParent.isValid() && collection.parentId() != newParent.id()
                && collection.resourceId() != newParent.resourceId()) {
            inhibitor.inhibit();
            ItemRetriever retriever(connection());
            retriever.setCollection(collection, true);
            retriever.setRetrieveFullPayload(true);
            if (!retriever.exec()) {
                throw HandlerException(retriever.lastError());
            }
        }
    }

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db);
    QList<QByteArray> changes;
    bool referencedChanged = false;

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::MimeTypes) {
        QStringList mts = cmd.mimeTypes();
        const MimeType::List currentMts = collection.mimeTypes();
        bool equal = true;
        for (const MimeType &currentMt : currentMts) {
            const int removeMts = mts.removeAll(currentMt.name());
            if (removeMts > 0) {
                continue;
            }
            equal = false;
            if (!collection.removeMimeType(currentMt)) {
                return failureResponse("Unable to remove collection mimetype");
            }
        }
        if (!db->appendMimeTypeForCollection(collection.id(), mts)) {
            return failureResponse("Unable to add collection mimetypes");
        }
        if (!equal || !mts.isEmpty()) {
            changes.append(AKONADI_PARAM_MIMETYPE);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::CachePolicy) {
        bool changed = false;
        const Protocol::CachePolicy newCp = cmd.cachePolicy();
        if (collection.cachePolicyCacheTimeout() != newCp.cacheTimeout()) {
            collection.setCachePolicyCacheTimeout(newCp.cacheTimeout());
            changed = true;
        }
        if (collection.cachePolicyCheckInterval() != newCp.checkInterval()) {
            collection.setCachePolicyCheckInterval(newCp.checkInterval());
            changed = true;
        }
        if (collection.cachePolicyInherit() != newCp.inherit()) {
            collection.setCachePolicyInherit(newCp.inherit());
            changed = true;
        }

        QStringList parts = newCp.localParts();
        qSort(parts);
        const QString localParts = parts.join(QLatin1Char(' '));
        if (collection.cachePolicyLocalParts() != localParts) {
            collection.setCachePolicyLocalParts(localParts);
            changed = true;
        }
        if (collection.cachePolicySyncOnDemand() != newCp.syncOnDemand()) {
            collection.setCachePolicySyncOnDemand(newCp.syncOnDemand());
            changed = true;
        }

        if (changed) {
            changes.append(AKONADI_PARAM_CACHEPOLICY);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::Name) {
        if (cmd.name() != collection.name()) {
            if (!CollectionQueryHelper::hasAllowedName(collection, cmd.name(), collection.parentId())) {
                return failureResponse("Collection with the same name exists already");
            }
            collection.setName(cmd.name());
            changes.append(AKONADI_PARAM_NAME);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::ParentID) {
        if (collection.parentId() != cmd.parentId()) {
            if (!db->moveCollection(collection, Collection::retrieveById(cmd.parentId()))) {
                return failureResponse("Unable to reparent collection");
            }
            changes.append(AKONADI_PARAM_PARENT);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::RemoteID) {
        if (cmd.remoteId() != collection.remoteId()) {
            if (!connection()->isOwnerResource(collection)) {
                return failureResponse("Only resources can modify remote identifiers");
            }
            collection.setRemoteId(cmd.remoteId());
            changes.append(AKONADI_PARAM_REMOTEID);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::RemoteRevision) {
        if (cmd.remoteRevision() != collection.remoteRevision()) {
            collection.setRemoteRevision(cmd.remoteRevision());
            changes.append(AKONADI_PARAM_REMOTEREVISION);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::PersistentSearch) {
        bool changed = false;
        if (cmd.persistentSearchQuery() != collection.queryString()) {
            collection.setQueryString(cmd.persistentSearchQuery());
            changed = true;
        }

        QList<QByteArray> queryAttributes = collection.queryAttributes().toUtf8().split(' ');
        if (cmd.persistentSearchRemote() != queryAttributes.contains(AKONADI_PARAM_REMOTE)) {
            if (cmd.persistentSearchRemote()) {
                queryAttributes.append(AKONADI_PARAM_REMOTE);
            } else {
                queryAttributes.removeOne(AKONADI_PARAM_REMOTE);
            }
            changed = true;
        }
        if (cmd.persistentSearchRecursive() != queryAttributes.contains(AKONADI_PARAM_RECURSIVE)) {
            if (cmd.persistentSearchRecursive()) {
                queryAttributes.append(AKONADI_PARAM_RECURSIVE);
            } else {
                queryAttributes.removeOne(AKONADI_PARAM_RECURSIVE);
            }
            changed = true;
        }
        if (changed) {
            collection.setQueryAttributes(QString::fromLatin1(queryAttributes.join(" ")));
        }

        QStringList cols;
        cols.reserve(cmd.persistentSearchCollections().size());
        QVector<qint64> inCols = cmd.persistentSearchCollections();
        qSort(inCols);
        Q_FOREACH (qint64 col, inCols) {
            cols.append(QString::number(col));
        }
        const QString colStr = cols.join(QLatin1Char(' '));
        if (colStr != collection.queryCollections()) {
            collection.setQueryCollections(colStr);
            changed = true;
        }

        if (changed || cmd.modifiedParts() & Protocol::ModifyCollectionCommand::MimeTypes) {
            SearchManager::instance()->updateSearch(collection);
            if (changed) {
                changes.append(AKONADI_PARAM_PERSISTENTSEARCH);
            }
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::ListPreferences) {
        if (cmd.enabled() != collection.enabled()) {
            collection.setEnabled(cmd.enabled());
            changes.append(AKONADI_PARAM_ENABLED);
        }
        if (cmd.syncPref() != collection.syncPref()) {
            collection.setSyncPref(cmd.syncPref());
            changes.append(AKONADI_PARAM_SYNC);
        }
        if (cmd.displayPref() != collection.displayPref()) {
            collection.setDisplayPref(cmd.displayPref());
            changes.append(AKONADI_PARAM_DISPLAY);
        }
        if (cmd.indexPref() != collection.indexPref()) {
            collection.setIndexPref(cmd.indexPref());
            changes.append(AKONADI_PARAM_INDEX);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::Referenced) {
        const bool wasReferencedFromSession = connection()->collectionReferenceManager()->isReferenced(collection.id(), connection()->sessionId());
        connection()->collectionReferenceManager()->referenceCollection(connection()->sessionId(), collection, cmd.referenced());
        const bool referenced = connection()->collectionReferenceManager()->isReferenced(collection.id());
        if (cmd.referenced() != wasReferencedFromSession) {
            changes.append(AKONADI_PARAM_REFERENCED);
        }
        if (referenced != collection.referenced()) {
            referencedChanged = true;
            collection.setReferenced(referenced);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::RemovedAttributes) {
        Q_FOREACH (const QByteArray &attr, cmd.removedAttributes()) {
            if (db->removeCollectionAttribute(collection, attr)) {
                changes.append(attr);
            }
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::Attributes) {
        const QMap<QByteArray, QByteArray> attrs = cmd.attributes();
        for (auto iter = attrs.cbegin(), end = attrs.cend(); iter != end; ++iter) {
            SelectQueryBuilder<CollectionAttribute> qb;
            qb.addValueCondition(CollectionAttribute::collectionIdColumn(), Query::Equals, collection.id());
            qb.addValueCondition(CollectionAttribute::typeColumn(), Query::Equals, iter.key());
            if (!qb.exec()) {
                return failureResponse("Unable to retrieve collection attribute");
            }

            const CollectionAttribute::List attrs = qb.result();
            if (attrs.isEmpty()) {
                CollectionAttribute newAttr;
                newAttr.setCollectionId(collection.id());
                newAttr.setType(iter.key());
                newAttr.setValue(iter.value());
                if (!newAttr.insert()) {
                    return failureResponse("Unable to add collection attribute");
                }
                changes.append(iter.key());
            } else if (attrs.size() == 1) {
                CollectionAttribute currAttr = attrs.first();
                if (currAttr.value() == iter.value()) {
                    continue;
                }
                currAttr.setValue(iter.value());
                if (!currAttr.update()) {
                    return failureResponse("Unable to update collection attribute");
                }
                changes.append(iter.key());
            } else {
                return failureResponse("WTF: more than one attribute with the same name");
            }
        }
    }

    if (!changes.isEmpty()) {
        if (collection.hasPendingChanges() && !collection.update()) {
            return failureResponse("Unable to update collection");
        }
        //This must be after the collection was updated in the db. The resource will immediately request a copy of the collection.
        if (AkonadiServer::instance()->intervalChecker() && collection.referenced() && referencedChanged) {
            AkonadiServer::instance()->intervalChecker()->requestCollectionSync(collection);
        }
        db->notificationCollector()->collectionChanged(collection, changes);
        //For backwards compatibility. Must be after the changed notification (otherwise the compression removes it).
        if (changes.contains(AKONADI_PARAM_ENABLED)) {
            if (collection.enabled()) {
                db->notificationCollector()->collectionSubscribed(collection);
            } else {
                db->notificationCollector()->collectionUnsubscribed(collection);
            }
        }
        if (!transaction.commit()) {
            return failureResponse("Unable to commit transaction");
        }
    }

    return successResponse<Protocol::ModifyCollectionResponse>();
}
