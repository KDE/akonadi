/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemmodifyjob.h"
#include "akonadicore_debug.h"
#include "itemmodifyjob_p.h"

#include "changemediator_p.h"
#include "collection.h"
#include "conflicthandler_p.h"
#include "item_p.h"
#include "itemserializer_p.h"
#include "job_p.h"

#include "gidextractor_p.h"
#include "protocolhelper_p.h"

#include <functional>

#include <QFile>

using namespace Akonadi;

ItemModifyJobPrivate::ItemModifyJobPrivate(ItemModifyJob *parent)
    : JobPrivate(parent)
{
}

void ItemModifyJobPrivate::setClean()
{
    mOperations.insert(Dirty);
}

Protocol::PartMetaData ItemModifyJobPrivate::preparePart(const QByteArray &partName)
{
    ProtocolHelper::PartNamespace ns; // dummy
    const QByteArray partLabel = ProtocolHelper::decodePartIdentifier(partName, ns);
    if (!mParts.contains(partLabel)) {
        // Error?
        return Protocol::PartMetaData();
    }

    mPendingData.clear();
    int version = 0;
    const auto item = mItems.first();
    if (mForeignParts.contains(partLabel)) {
        mPendingData = item.d_ptr->mPayloadPath.toUtf8();
        const auto size = QFile(item.d_ptr->mPayloadPath).size();
        return Protocol::PartMetaData(partName, size, version, Protocol::PartMetaData::Foreign);
    } else {
        ItemSerializer::serialize(mItems.first(), partLabel, mPendingData, version);
        return Protocol::PartMetaData(partName, mPendingData.size(), version);
    }
}

void ItemModifyJobPrivate::conflictResolved()
{
    Q_Q(ItemModifyJob);

    q->setError(KJob::NoError);
    q->setErrorText(QString());
    q->emitResult();
}

void ItemModifyJobPrivate::conflictResolveError(const QString &message)
{
    Q_Q(ItemModifyJob);

    q->setErrorText(q->errorText() + message);
    q->emitResult();
}

void ItemModifyJobPrivate::doUpdateItemRevision(Akonadi::Item::Id itemId, int oldRevision, int newRevision)
{
    auto it = std::find_if(mItems.begin(), mItems.end(), [&itemId](const Item &item) -> bool {
        return item.id() == itemId;
    });
    if (it != mItems.end() && (*it).revision() == oldRevision) {
        (*it).setRevision(newRevision);
    }
}

QString ItemModifyJobPrivate::jobDebuggingString() const
{
    try {
        return Protocol::debugString(fullCommand());
    } catch (const Exception &e) {
        return QString::fromUtf8(e.what());
    }
}

void ItemModifyJobPrivate::setSilent(bool silent)
{
    mSilent = silent;
}

ItemModifyJob::ItemModifyJob(const Item &item, QObject *parent)
    : Job(new ItemModifyJobPrivate(this), parent)
{
    Q_D(ItemModifyJob);

    d->mItems.append(item);
    d->mParts = item.loadedPayloadParts();

    d->mOperations.insert(ItemModifyJobPrivate::RemoteId);
    d->mOperations.insert(ItemModifyJobPrivate::RemoteRevision);

    if (!item.payloadPath().isEmpty()) {
        d->mForeignParts = ItemSerializer::allowedForeignParts(item);
    }
}

ItemModifyJob::ItemModifyJob(const Akonadi::Item::List &items, QObject *parent)
    : Job(new ItemModifyJobPrivate(this), parent)
{
    Q_ASSERT(!items.isEmpty());
    Q_D(ItemModifyJob);
    d->mItems = items;

    // same as single item ctor
    if (d->mItems.size() == 1) {
        d->mParts = items.first().loadedPayloadParts();
        d->mOperations.insert(ItemModifyJobPrivate::RemoteId);
        d->mOperations.insert(ItemModifyJobPrivate::RemoteRevision);
    } else {
        d->mIgnorePayload = true;
        d->mRevCheck = false;
    }
}

ItemModifyJob::~ItemModifyJob()
{
}

Protocol::ModifyItemsCommandPtr ItemModifyJobPrivate::fullCommand() const
{
    auto cmd = Protocol::ModifyItemsCommandPtr::create();

    const Akonadi::Item item = mItems.first();
    for (int op : std::as_const(mOperations)) {
        switch (op) {
        case ItemModifyJobPrivate::RemoteId:
            if (!item.remoteId().isNull()) {
                cmd->setRemoteId(item.remoteId());
            }
            break;
        case ItemModifyJobPrivate::Gid: {
            const QString gid = GidExtractor::getGid(item);
            if (!gid.isNull()) {
                cmd->setGid(gid);
            }
            break;
        }
        case ItemModifyJobPrivate::RemoteRevision:
            if (!item.remoteRevision().isNull()) {
                cmd->setRemoteRevision(item.remoteRevision());
            }
            break;
        case ItemModifyJobPrivate::Dirty:
            cmd->setDirty(false);
            break;
        }
    }

    if (item.d_ptr->mClearPayload) {
        cmd->setInvalidateCache(true);
    }
    if (mSilent) {
        cmd->setNotify(true);
    }

    if (item.d_ptr->mFlagsOverwritten) {
        cmd->setFlags(item.flags());
    } else {
        const auto addedFlags = ItemChangeLog::instance()->addedFlags(item.d_ptr);
        if (!addedFlags.isEmpty()) {
            cmd->setAddedFlags(addedFlags);
        }
        const auto deletedFlags = ItemChangeLog::instance()->deletedFlags(item.d_ptr);
        if (!deletedFlags.isEmpty()) {
            cmd->setRemovedFlags(deletedFlags);
        }
    }

    if (item.d_ptr->mTagsOverwritten) {
        const auto tags = item.tags();
        if (!tags.isEmpty()) {
            cmd->setTags(ProtocolHelper::entitySetToScope(tags));
        }
    } else {
        const auto addedTags = ItemChangeLog::instance()->addedTags(item.d_ptr);
        if (!addedTags.isEmpty()) {
            cmd->setAddedTags(ProtocolHelper::entitySetToScope(addedTags));
        }
        const auto deletedTags = ItemChangeLog::instance()->deletedTags(item.d_ptr);
        if (!deletedTags.isEmpty()) {
            cmd->setRemovedTags(ProtocolHelper::entitySetToScope(deletedTags));
        }
    }

    if (!mParts.isEmpty()) {
        QSet<QByteArray> parts;
        parts.reserve(mParts.size());
        for (const QByteArray &part : std::as_const(mParts)) {
            parts.insert(ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartPayload, part));
        }
        cmd->setParts(parts);
    }

    const AttributeStorage &attributeStorage = ItemChangeLog::instance()->attributeStorage(item.d_ptr);
    const QSet<QByteArray> deletedAttributes = attributeStorage.deletedAttributes();
    if (!deletedAttributes.isEmpty()) {
        QSet<QByteArray> removedParts;
        removedParts.reserve(deletedAttributes.size());
        for (const QByteArray &part : deletedAttributes) {
            removedParts.insert("ATR:" + part);
        }
        cmd->setRemovedParts(removedParts);
    }
    if (attributeStorage.hasModifiedAttributes()) {
        cmd->setAttributes(ProtocolHelper::attributesToProtocol(attributeStorage.modifiedAttributes()));
    }

    // nothing to do
    if (cmd->modifiedParts() == Protocol::ModifyItemsCommand::None && mParts.isEmpty() && !cmd->invalidateCache()) {
        return cmd;
    }

    cmd->setItems(ProtocolHelper::entitySetToScope(mItems));
    if (mRevCheck && item.revision() >= 0) {
        cmd->setOldRevision(item.revision());
    }

    if (item.d_ptr->mSizeChanged) {
        cmd->setItemSize(item.size());
    }

    return cmd;
}

void ItemModifyJob::doStart()
{
    Q_D(ItemModifyJob);

    Protocol::ModifyItemsCommandPtr command;
    try {
        command = d->fullCommand();
    } catch (const Exception &e) {
        setError(Job::Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }

    if (command->modifiedParts() == Protocol::ModifyItemsCommand::None) {
        emitResult();
        return;
    }

    d->sendCommand(command);
}

bool ItemModifyJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(ItemModifyJob);

    if (!response->isResponse() && response->type() == Protocol::Command::StreamPayload) {
        const auto &streamCmd = Protocol::cmdCast<Protocol::StreamPayloadCommand>(response);
        auto streamResp = Protocol::StreamPayloadResponsePtr::create();
        if (streamCmd.request() == Protocol::StreamPayloadCommand::MetaData) {
            streamResp->setMetaData(d->preparePart(streamCmd.payloadName()));
        } else {
            if (streamCmd.destination().isEmpty()) {
                streamResp->setData(d->mPendingData);
            } else {
                QByteArray error;
                if (!ProtocolHelper::streamPayloadToFile(streamCmd.destination(), d->mPendingData, error)) {
                    // TODO: Error?
                }
            }
        }
        d->sendCommand(tag, streamResp);
        return false;
    }

    if (response->isResponse() && response->type() == Protocol::Command::ModifyItems) {
        const auto &resp = Protocol::cmdCast<Protocol::ModifyItemsResponse>(response);
        if (resp.errorCode()) {
            setError(Unknown);
            setErrorText(resp.errorMessage());
            return true;
        }

        if (resp.errorMessage().contains(QLatin1String("[LLCONFLICT]"))) {
            if (d->mAutomaticConflictHandlingEnabled) {
                auto handler = new ConflictHandler(ConflictHandler::LocalLocalConflict, this);
                handler->setConflictingItems(d->mItems.first(), d->mItems.first());
                connect(handler, &ConflictHandler::conflictResolved, this, [d]() {
                    d->conflictResolved();
                });
                connect(handler, &ConflictHandler::error, this, [d](const QString &str) {
                    d->conflictResolveError(str);
                });
                QMetaObject::invokeMethod(handler, &ConflictHandler::start, Qt::QueuedConnection);
                return true;
            }
        }

        if (resp.modificationDateTime().isValid()) {
            Item &item = d->mItems.first();
            item.setModificationTime(resp.modificationDateTime());
            item.d_ptr->resetChangeLog();
        } else if (resp.id() > -1) {
            auto it = std::find_if(d->mItems.begin(), d->mItems.end(), [&resp](const Item &item) -> bool {
                return item.id() == resp.id();
            });
            if (it == d->mItems.end()) {
                qCDebug(AKONADICORE_LOG) << "Received STORE response for an item we did not modify: " << tag << Protocol::debugString(response);
                return true;
            }

            const int newRev = resp.newRevision();
            const int oldRev = (*it).revision();
            if (newRev >= oldRev && newRev >= 0) {
                d->itemRevisionChanged((*it).id(), oldRev, newRev);
                (*it).setRevision(newRev);
            }
            // There will be more responses, either for other modified items,
            // or the final response with invalid ID, but with modification datetime
            return false;
        }

        for (const Item &item : std::as_const(d->mItems)) {
            ChangeMediator::invalidateItem(item);
        }

        return true;
    }

    return Job::doHandleResponse(tag, response);
}

void ItemModifyJob::setIgnorePayload(bool ignore)
{
    Q_D(ItemModifyJob);

    if (d->mIgnorePayload == ignore) {
        return;
    }

    d->mIgnorePayload = ignore;
    if (d->mIgnorePayload) {
        d->mParts = QSet<QByteArray>();
    } else {
        Q_ASSERT(!d->mItems.first().mimeType().isEmpty());
        d->mParts = d->mItems.first().loadedPayloadParts();
    }
}

bool ItemModifyJob::ignorePayload() const
{
    Q_D(const ItemModifyJob);

    return d->mIgnorePayload;
}

void ItemModifyJob::setUpdateGid(bool update)
{
    Q_D(ItemModifyJob);
    if (update && !updateGid()) {
        d->mOperations.insert(ItemModifyJobPrivate::Gid);
    } else {
        d->mOperations.remove(ItemModifyJobPrivate::Gid);
    }
}

bool ItemModifyJob::updateGid() const
{
    Q_D(const ItemModifyJob);
    return d->mOperations.contains(ItemModifyJobPrivate::Gid);
}

void ItemModifyJob::disableRevisionCheck()
{
    Q_D(ItemModifyJob);

    d->mRevCheck = false;
}

void ItemModifyJob::disableAutomaticConflictHandling()
{
    Q_D(ItemModifyJob);

    d->mAutomaticConflictHandlingEnabled = false;
}

Item ItemModifyJob::item() const
{
    Q_D(const ItemModifyJob);
    Q_ASSERT(d->mItems.size() == 1);

    return d->mItems.first();
}

Item::List ItemModifyJob::items() const
{
    Q_D(const ItemModifyJob);
    return d->mItems;
}

#include "moc_itemmodifyjob.cpp"
