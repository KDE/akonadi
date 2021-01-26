/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionmodifyhandler.h"

#include "akonadi.h"
#include "akonadiserver_debug.h"
#include "cachecleaner.h"
#include "connection.h"
#include "handlerhelper.h"
#include "intervalcheck.h"
#include "search/searchmanager.h"
#include "shared/akranges.h"
#include "storage/collectionqueryhelper.h"
#include "storage/datastore.h"
#include "storage/itemretriever.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

using namespace Akonadi;
using namespace Akonadi::Server;
using namespace AkRanges;

CollectionModifyHandler::CollectionModifyHandler(AkonadiServer &akonadi)
    : Handler(akonadi)
{
}

bool CollectionModifyHandler::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::ModifyCollectionCommand>(m_command);

    Collection collection = HandlerHelper::collectionFromScope(cmd.collection(), connection()->context());
    if (!collection.isValid()) {
        return failureResponse("No such collection");
    }

    CacheCleanerInhibitor inhibitor(akonadi(), false);

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::ParentID) {
        const Collection newParent = Collection::retrieveById(cmd.parentId());
        if (newParent.isValid() && collection.parentId() != newParent.id() && collection.resourceId() != newParent.resourceId()) {
            inhibitor.inhibit();
            ItemRetriever retriever(akonadi().itemRetrievalManager(), connection(), connection()->context());
            retriever.setCollection(collection, true);
            retriever.setRetrieveFullPayload(true);
            if (!retriever.exec()) {
                throw HandlerException(retriever.lastError());
            }
        }
    }

    DataStore *db = connection()->storageBackend();
    Transaction transaction(db, QStringLiteral("MODIFY"));
    QList<QByteArray> changes;

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
        std::sort(parts.begin(), parts.end());
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
        if (cmd.remoteId() != collection.remoteId() && !cmd.remoteId().isEmpty()) {
            if (!connection()->isOwnerResource(collection)) {
                qCWarning(AKONADISERVER_LOG) << "Invalid attempt to modify the collection remoteID from" << collection.remoteId() << "to" << cmd.remoteId();
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
            collection.setQueryAttributes(QString::fromLatin1(queryAttributes.join(' ')));
        }

        QVector<qint64> inCols = cmd.persistentSearchCollections();
        std::sort(inCols.begin(), inCols.end());
        const auto cols = inCols | Views::transform([](const auto col) {
                              return QString::number(col);
                          })
            | Actions::toQList;
        const QString colStr = cols.join(QLatin1Char(' '));
        if (colStr != collection.queryCollections()) {
            collection.setQueryCollections(colStr);
            changed = true;
        }

        if (changed || cmd.modifiedParts() & Protocol::ModifyCollectionCommand::MimeTypes) {
            changes.append(AKONADI_PARAM_PERSISTENTSEARCH);
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyCollectionCommand::ListPreferences) {
        if (cmd.enabled() != collection.enabled()) {
            collection.setEnabled(cmd.enabled());
            changes.append(AKONADI_PARAM_ENABLED);
        }
        if (cmd.syncPref() != static_cast<Tristate>(collection.syncPref())) {
            collection.setSyncPref(static_cast<Collection::Tristate>(cmd.syncPref()));
            changes.append(AKONADI_PARAM_SYNC);
        }
        if (cmd.displayPref() != static_cast<Tristate>(collection.displayPref())) {
            collection.setDisplayPref(static_cast<Collection::Tristate>(cmd.displayPref()));
            changes.append(AKONADI_PARAM_DISPLAY);
        }
        if (cmd.indexPref() != static_cast<Tristate>(collection.indexPref())) {
            collection.setIndexPref(static_cast<Collection::Tristate>(cmd.indexPref()));
            changes.append(AKONADI_PARAM_INDEX);
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
        db->notificationCollector()->collectionChanged(collection, changes);
        // For backwards compatibility. Must be after the changed notification (otherwise the compression removes it).
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

        // Only request Search update AFTER committing the transaction to avoid
        // transaction deadlock with SQLite
        if (changes.contains(AKONADI_PARAM_PERSISTENTSEARCH)) {
            akonadi().searchManager().updateSearch(collection);
        }
    }

    return successResponse<Protocol::ModifyCollectionResponse>();
}
