/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "store.h"

#include "connection.h"
#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/transaction.h"
#include "storage/itemqueryhelper.h"
#include "storage/selectquerybuilder.h"
#include "storage/parthelper.h"
#include "storage/dbconfig.h"
#include "storage/itemretriever.h"
#include "storage/parttypehelper.h"
#include "storage/partstreamer.h"
#include <private/externalpartstorage_p.h>


#include "akonadiserver_debug.h"

#include <algorithm>
#include <functional>

using namespace Akonadi;
using namespace Akonadi::Server;

static bool payloadChanged(const QSet<QByteArray> &changes)
{
    for (const QByteArray &change : changes) {
        if (change.startsWith(AKONADI_PARAM_PLD)) {
            return true;
        }
    }
    return false;
}

bool Store::replaceFlags(const PimItem::List &item, const QSet<QByteArray> &flags, bool &flagsChanged)
{
    Flag::List flagList = HandlerHelper::resolveFlags(flags);
    DataStore *store = connection()->storageBackend();

    if (!store->setItemsFlags(item, flagList, &flagsChanged)) {
        qCWarning(AKONADISERVER_LOG) << "Store::replaceFlags: Unable to replace flags";
        return false;
    }

    return true;
}

bool Store::addFlags(const PimItem::List &items, const QSet<QByteArray> &flags, bool &flagsChanged)
{
    const Flag::List flagList = HandlerHelper::resolveFlags(flags);
    DataStore *store = connection()->storageBackend();

    if (!store->appendItemsFlags(items, flagList, &flagsChanged)) {
        qCWarning(AKONADISERVER_LOG) << "Store::addFlags: Unable to add new item flags";
        return false;
    }
    return true;
}

bool Store::deleteFlags(const PimItem::List &items, const QSet<QByteArray> &flags, bool &flagsChanged)
{
    DataStore *store = connection()->storageBackend();

    QVector<Flag> flagList;
    flagList.reserve(flags.size());
    for (auto iter = flags.cbegin(), end = flags.cend(); iter != end; ++iter) {
        Flag flag = Flag::retrieveByName(QString::fromUtf8(*iter));
        if (!flag.isValid()) {
            continue;
        }

        flagList.append(flag);
    }

    if (!store->removeItemsFlags(items, flagList, &flagsChanged)) {
        qCWarning(AKONADISERVER_LOG) << "Store::deleteFlags: Unable to remove item flags";
        return false;
    }
    return true;
}

bool Store::replaceTags(const PimItem::List &item, const Scope &tags, bool &tagsChanged)
{
    const Tag::List tagList = HandlerHelper::tagsFromScope(tags, connection());
    if (!connection()->storageBackend()->setItemsTags(item, tagList, &tagsChanged)) {
        qCWarning(AKONADISERVER_LOG) << "Store::replaceTags: unable to replace tags";
        return false;
    }
    return true;
}

bool Store::addTags(const PimItem::List &items, const Scope &tags, bool &tagsChanged)
{
    const Tag::List tagList = HandlerHelper::tagsFromScope(tags, connection());
    if (!connection()->storageBackend()->appendItemsTags(items, tagList, &tagsChanged)) {
        qCWarning(AKONADISERVER_LOG) << "Store::addTags: Unable to add new item tags";
        return false;
    }
    return true;
}

bool Store::deleteTags(const PimItem::List &items, const Scope &tags, bool &tagsChanged)
{
    const Tag::List tagList = HandlerHelper::tagsFromScope(tags, connection());
    if (!connection()->storageBackend()->removeItemsTags(items, tagList, &tagsChanged)) {
        qCWarning(AKONADISERVER_LOG) << "Store::deleteTags: Unable to remove item tags";
        return false;
    }
    return true;
}

bool Store::parseStream()
{
    const auto &cmd = Protocol::cmdCast<Protocol::ModifyItemsCommand>(m_command);

    //parseCommand();

    DataStore *store = connection()->storageBackend();
    Transaction transaction(store, QStringLiteral("STORE"));
    ExternalPartStorageTransaction storageTrx;
    // Set the same modification time for each item.
    QDateTime modificationtime = QDateTime::currentDateTimeUtc();
    if (DbType::type(store->database()) != DbType::Sqlite) {
        // Remove milliseconds from the modificationtime. PSQL and MySQL don't
        // support milliseconds in DATETIME column, so FETCHed Items will report
        // time without milliseconds, while this command would return answer
        // with milliseconds
        modificationtime = modificationtime.addMSecs(-modificationtime.time().msec());
    }

    // retrieve selected items
    SelectQueryBuilder<PimItem> qb;
    qb.setForUpdate();
    ItemQueryHelper::scopeToQuery(cmd.items(), connection()->context(), qb);
    if (!qb.exec()) {
        return failureResponse("Unable to retrieve items");
    }
    PimItem::List pimItems = qb.result();
    if (pimItems.isEmpty()) {
        return failureResponse("No items found");
    }

    for (int i = 0; i < pimItems.size(); ++i) {
        if (cmd.oldRevision() > -1) {
            // check for conflicts if a resources tries to overwrite an item with dirty payload
            const PimItem &pimItem = pimItems.at(i);
            if (connection()->isOwnerResource(pimItem)) {
                if (pimItem.dirty()) {
                    const QString error = QStringLiteral("[LRCONFLICT] Resource %1 tries to modify item %2 (%3) (in collection %4) with dirty payload, aborting STORE.");
                    return failureResponse(
                               error.arg(pimItem.collection().resource().name())
                               .arg(pimItem.id())
                               .arg(pimItem.remoteId()).arg(pimItem.collectionId()));
                }
            }

            // check and update revisions
            if (pimItems.at(i).rev() != (int) cmd.oldRevision()) {
                const QString error = QStringLiteral("[LLCONFLICT] Resource %1 tries to modify item %2 (%3) (in collection %4) with revision %5; the item was modified elsewhere and has revision %6, aborting STORE.");
                return failureResponse(error.arg(pimItem.collection().resource().name())
                                            .arg(pimItem.id())
                                            .arg(pimItem.remoteId()).arg(pimItem.collectionId())
                                            .arg(cmd.oldRevision()).arg(pimItems.at(i).rev()));
            }
        }
    }

    PimItem &item = pimItems.first();

    QSet<QByteArray> changes;
    qint64 partSizes = 0;
    qint64 size = 0;

    bool flagsChanged = false;
    bool tagsChanged = false;

    if (cmd.modifiedParts() & Protocol::ModifyItemsCommand::AddedFlags) {
        if (!addFlags(pimItems, cmd.addedFlags(), flagsChanged)) {
            return failureResponse("Unable to add item flags");
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyItemsCommand::RemovedFlags) {
        if (!deleteFlags(pimItems, cmd.removedFlags(), flagsChanged)) {
            return failureResponse("Unable to remove item flags");
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyItemsCommand::Flags) {
        if (!replaceFlags(pimItems, cmd.flags(), flagsChanged)) {
            return failureResponse("Unable to reset flags");
        }
    }

    if (flagsChanged) {
        changes << AKONADI_PARAM_FLAGS;
    }

    if (cmd.modifiedParts() & Protocol::ModifyItemsCommand::AddedTags) {
        if (!addTags(pimItems, cmd.addedTags(), tagsChanged)) {
            return failureResponse("Unable to add item tags");
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyItemsCommand::RemovedTags) {
        if (!deleteTags(pimItems, cmd.removedTags(), tagsChanged)) {
            return failureResponse("Unable to remove item tags");
        }
    }

    if (cmd.modifiedParts() & Protocol::ModifyItemsCommand::Tags) {
        if (!replaceTags(pimItems, cmd.tags(), tagsChanged)) {
            return failureResponse("Unable to reset item tags");
        }
    }

    if (tagsChanged) {
        changes << AKONADI_PARAM_TAGS;
    }

    if (item.isValid() && cmd.modifiedParts() & Protocol::ModifyItemsCommand::RemoteID) {
        if (item.remoteId() != cmd.remoteId() && !cmd.remoteId().isEmpty()) {
            if (!connection()->isOwnerResource(item)) {
                qCWarning(AKONADISERVER_LOG) << "Invalid attempt to modify the remoteID for item" << item.id() << "from" << item.remoteId() << "to" << cmd.remoteId();
                return failureResponse("Only resources can modify remote identifiers");
            }
            item.setRemoteId(cmd.remoteId());
            changes << AKONADI_PARAM_REMOTEID;
        }
    }

    if (item.isValid() && cmd.modifiedParts() & Protocol::ModifyItemsCommand::GID) {
        if (item.gid() != cmd.gid()) {
            item.setGid(cmd.gid());
        }
        changes << AKONADI_PARAM_GID;
    }

    if (item.isValid() && cmd.modifiedParts() & Protocol::ModifyItemsCommand::RemoteRevision) {
        if (item.remoteRevision() != cmd.remoteRevision()) {
            if (!connection()->isOwnerResource(item)) {
                return failureResponse("Only resources can modify remote revisions");
            }
            item.setRemoteRevision(cmd.remoteRevision());
            changes << AKONADI_PARAM_REMOTEREVISION;
        }
    }

    if (item.isValid() && !cmd.dirty()) {
        item.setDirty(false);
    }

    if (item.isValid() && cmd.modifiedParts() & Protocol::ModifyItemsCommand::Size) {
        size = cmd.itemSize();
        changes << AKONADI_PARAM_SIZE;
    }

    if (item.isValid() && cmd.modifiedParts() & Protocol::ModifyItemsCommand::RemovedParts) {
        if (!cmd.removedParts().isEmpty()) {
            if (!store->removeItemParts(item, cmd.removedParts())) {
                return failureResponse("Unable to remove item parts");
            }
            Q_FOREACH (const QByteArray &part, cmd.removedParts()) {
                changes.insert(part);
            }
        }
    }

    if (item.isValid() && cmd.modifiedParts() & Protocol::ModifyItemsCommand::Parts) {
        PartStreamer streamer(connection(), item);
        Q_FOREACH (const QByteArray &partName, cmd.parts()) {
            qint64 partSize = 0;
            try {
                streamer.stream(true, partName, partSize);
            } catch (const PartStreamerException &e) {
                return failureResponse(e.what());
            }

            changes.insert(partName);
            partSizes += partSize;
        }
    }

    if (item.isValid() && cmd.modifiedParts() & Protocol::ModifyItemsCommand::Attributes) {
        PartStreamer streamer(connection(), item);
        const Protocol::Attributes attrs = cmd.attributes();
        for (auto iter = attrs.cbegin(), end = attrs.cend(); iter != end; ++iter) {
            bool changed = false;
            try {
                streamer.streamAttribute(true, iter.key(), iter.value(), &changed);
            } catch (const PartStreamerException &e) {
                return failureResponse(e.what());
            }

            if (changed) {
                changes.insert(iter.key());
            }
        }
    }

    QDateTime datetime;
    if (!changes.isEmpty() || cmd.invalidateCache() || !cmd.dirty()) {

        // update item size
        if (pimItems.size() == 1 && (size > 0 || partSizes > 0)) {
            pimItems.first().setSize(qMax(size, partSizes));
        }

        const bool onlyRemoteIdChanged = (changes.size() == 1 && changes.contains(AKONADI_PARAM_REMOTEID));
        const bool onlyRemoteRevisionChanged = (changes.size() == 1 && changes.contains(AKONADI_PARAM_REMOTEREVISION));
        const bool onlyRemoteIdAndRevisionChanged = (changes.size() == 2 && changes.contains(AKONADI_PARAM_REMOTEID)
                && changes.contains(AKONADI_PARAM_REMOTEREVISION));
        const bool onlyFlagsChanged = (changes.size() == 1 && changes.contains(AKONADI_PARAM_FLAGS));
        const bool onlyGIDChanged = (changes.size() == 1 && changes.contains(AKONADI_PARAM_GID));
        // If only the remote id and/or the remote revision changed, we don't have to increase the REV,
        // because these updates do not change the payload and can only be done by the owning resource -> no conflicts possible
        const bool revisionNeedsUpdate = (!changes.isEmpty() && !onlyRemoteIdChanged && !onlyRemoteRevisionChanged && !onlyRemoteIdAndRevisionChanged && !onlyGIDChanged);

        // run update query and prepare change notifications
        for (int i = 0; i < pimItems.count(); ++i) {

            if (revisionNeedsUpdate) {
                pimItems[i].setRev(pimItems[i].rev() + 1);
            }

            PimItem &item = pimItems[i];
            item.setDatetime(modificationtime);
            item.setAtime(modificationtime);
            if (!connection()->isOwnerResource(item) && payloadChanged(changes)) {
                item.setDirty(true);
            }
            if (!item.update()) {
                return failureResponse("Unable to write item changes into the database");
            }

            if (cmd.invalidateCache()) {
                if (!store->invalidateItemCache(item)) {
                    return failureResponse("Unable to invalidate item cache in the database");
                }
            }

            // flags change notification went separately during command parsing
            // GID-only changes are ignored to prevent resources from updating their storage when no actual change happened
            if (cmd.notify() && !changes.isEmpty() && !onlyFlagsChanged && !onlyGIDChanged) {
                // Don't send FLAGS notification in itemChanged
                changes.remove(AKONADI_PARAM_FLAGS);
                store->notificationCollector()->itemChanged(item, changes);
            }

            if (!cmd.noResponse()) {
                Protocol::ModifyItemsResponse resp;
                resp.setId(item.id());
                resp.setNewRevision(item.rev());
                sendResponse(std::move(resp));
            }
        }

        if (!transaction.commit()) {
            return failureResponse("Cannot commit transaction.");
        }
        // Always commit storage changes (deletion) after DB transaction
        storageTrx.commit();

        datetime = modificationtime;
    } else {
        datetime = pimItems.first().datetime();
    }

    Protocol::ModifyItemsResponse resp;
    resp.setModificationDateTime(datetime);
    return successResponse(std::move(resp));
}
