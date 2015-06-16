/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemmodifyjob.h"
#include "itemmodifyjob_p.h"

#include "changemediator_p.h"
#include "collection.h"
#include "conflicthandler_p.h"
#include "entity_p.h"
#include "item_p.h"
#include "itemserializer_p.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "gidextractor_p.h"

#include <functional>

using namespace Akonadi;

ItemModifyJobPrivate::ItemModifyJobPrivate(ItemModifyJob *parent)
    : JobPrivate(parent)
    , mRevCheck(true)
    , mIgnorePayload(false)
    , mAutomaticConflictHandlingEnabled(true)
    , mSilent(false)
{
}

void ItemModifyJobPrivate::setClean()
{
    mOperations.insert(Dirty);
}

Protocol::PartMetaData ItemModifyJobPrivate::preparePart(const QByteArray &partName)
{
    if (!mParts.remove(partName)) {
        // Error?
        return Protocol::PartMetaData();
    }

    mPendingData.clear();
    int version = 0;
    ItemSerializer::serialize(mItems.first(), partName, mPendingData, version);

    return Protocol::PartMetaData(partName, mPendingData.size(), version);
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
    using namespace std::placeholders;
    Item::List::iterator it = std::find_if(mItems.begin(), mItems.end(), std::bind(&Item::id, _1) == itemId);
    if (it != mItems.end() && (*it).revision() == oldRevision) {
        (*it).setRevision(newRevision);
    }
}

QString ItemModifyJobPrivate::jobDebuggingString() const
{
    try {
        return QString::fromUtf8(fullCommand());
    } catch (const Exception &e) {
        return QString::fromUtf8(e.what());
    }
}

void ItemModifyJobPrivate::setSilent( bool silent )
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

QByteArray ItemModifyJobPrivate::fullCommand() const
{
    Protocol::ModifyItemsCommand cmd;

    const Akonadi::Item item = mItems.first();
    QList<QByteArray> changes;
    foreach (int op, mOperations) {
        switch (op) {
        case ItemModifyJobPrivate::RemoteId:
            if (!item.remoteId().isNull()) {
                cmd.setRemoteId(item.remoteId());
            }
            break;
        case ItemModifyJobPrivate::Gid: {
            const QString gid = GidExtractor::getGid(item);
            if (!gid.isNull()) {
                cmd.setGid(gid);
            }
            break;
        }
        case ItemModifyJobPrivate::RemoteRevision:
            if (!item.remoteRevision().isNull()) {
                cmd.setRemoteRevision(item.remoteRevision());
            }
            break;
        case ItemModifyJobPrivate::Dirty:
            cmd.setDirty(false);
            break;
        }
    }

    if (item.d_func()->mClearPayload) {
        cmd.setInvalidateCache(true);
    }
    if (mSilent) {
        cmd.setNotify(true);
    }

    if (item.d_func()->mFlagsOverwritten) {
        cmd.setFlags(item.flags());
    } else {
        if (!item.d_func()->mAddedFlags.isEmpty()) {
            cmd.setAddedFlags(item.d_func()->mAddedFlags);
        }
        if (!item.d_func()->mDeletedFlags.isEmpty()) {
            cmd.setRemovedFlags(item.d_func()->mDeletedFlags);
        }
    }

    if (item.d_func()->mTagsOverwritten) {
        cmd.setTags(ProtocolHelper::entitySetToScope(item.tags()));
    } else {
        if (!item.d_func()->mAddedTags.isEmpty()) {
            cmd.setAddedTags(ProtocolHelper::entitySetToScope(item.d_func()->mAddedTags));
        }
        if (!item.d_func()->mDeletedTags.isEmpty()) {
            cmd.setRemovedFlags(ProtocolHelper::entitySetToScope(item.d_func()->mDeletedTags));
        }
    }

    if (!item.d_func()->mDeletedAttributes.isEmpty()) {
        cmd.setRemovedParts(item.d_func()->mDeletedAttributes);
    }

    // nothing to do
    if (cmd.modifiedParts() == Protocol::ModifyItemsCommand::None) {
        return cmd;
    }

    cmd.setItems(ProtocolHelper::entitySetToScope(mItems));
    if (mRevCheck || item.revision() >= 0) {
        cmd.setOldRevision(item.revision());
    }

    if (item.d_func()->mSizeChanged) {
        cmd.setItemSize(item.size());
    }

    cmd.setAttributes(ProtocolHelper::attributesToProtocol(item));
    return cmd;
}

void ItemModifyJob::doStart()
{
    Q_D(ItemModifyJob);

    Protocol::ModifyItemsCommand command;
    try {
        command = d->fullCommand();
    } catch (const Exception &e) {
        setError(Job::Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }

    if (command.modifiedParts() == Protocol::ModifyItemsCommand::None) {
        emitResult();
        return;
    }

    d->sendCommand(command);
}

void ItemModifyJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    Q_D(ItemModifyJob);

    if (!response.isResponse() && response.type() == Protocol::Command::StreamPayload) {
        const Protocol::StreamPayloadCommand streamCmd(response);
        Protocol::StreamPayloadResponse streamResp;
        if (streamCmd.request() == Protocol::StreamPayloadCommand::MetaData) {
            streamResp.setMetaData(d->preparePart(streamCmd.payloadName());
        } else {
            if (streamCmd.destination().isEmpty()) {
                streamResp.setData(d->mPendingData);
            } else {
                ProtocolHelper::streamPayloadToFile(streamCmd.destination(), d->mPendingData, error);
            }
        }
        d->sendCommand(streamResp);
    } else if (response.isResponse() && response.type() == Protocol::Command::ModifyItems) {
        // FIXME: Conflict handling
        /*
            setError(Unknown);
            setErrorText(QString::fromUtf8(data));

            if (data.contains("[LLCONFLICT]")) {
                if (d->mAutomaticConflictHandlingEnabled) {
                    ConflictHandler *handler = new ConflictHandler(ConflictHandler::LocalLocalConflict, this);
                    handler->setConflictingItems(d->mItems.first(), d->mItems.first());
                    connect(handler, SIGNAL(conflictResolved()), SLOT(conflictResolved()));
                    connect(handler, SIGNAL(error(QString)), SLOT(conflictResolveError(QString)));

                    QMetaObject::invokeMethod(handler, "start", Qt::QueuedConnection);
                    return;
                }
            }
        */

        Protocol::ModifyItemsResponse resp(response);
        if (resp.modificationDateTime().isValid()) {
            Item &item = d->mItems.first();
            item.setModificationTime(resp.modificationDateTime());
            item.d_ptr->resetChangeLog();
        } else if (resp.id() > -1) {
            using namespace std::placeholders;
            Item::List::Iterator it = std::find_if(d->mItems.constBegin(), d->mItems.constEnd(),
                                                   std::bind(&Item::id, _1) == resp.id());
            if (it == d->mItems.constEnd()) {
                qDebug() << "Received STORE response for an item we did not modify: " << tag << response.debugString();
                return;
            }

            const int newRev = resp.newRevision();
            const int oldRev = (*it).revision();
            if (newRev < oldRev || newRev < 0) {
                continue;
            }
            d->itemRevisionChanged((*it).id(), oldRev, newRev);
            (*it).setRevision(newRev);
        }

        for (const Item &item : d->mItems) {
            ChangeMediator::invalidateItem(item);
        }

        emitResult();
    } else {
        Job::doHandleResponse(tag, response);
    }
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
