/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007        Robert Zwerus <arzie@dds.nl>
    Copyright (c) 2014        Daniel Vrátil <dvratil@redhat.com>

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

#include "itemcreatejob.h"

#include "collection.h"
#include "item.h"
#include "item_p.h"
#include "itemserializer_p.h"
#include "job_p.h"
#include "protocolhelper_p.h"
#include "gidextractor_p.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>

#include <akonadi/private/protocol_p.h>

using namespace Akonadi;

class Akonadi::ItemCreateJobPrivate : public JobPrivate
{
public:
    ItemCreateJobPrivate(ItemCreateJob *parent)
        : JobPrivate(parent)
        , mMergeOptions(ItemCreateJob::NoMerge)
        , mItemReceived(false)
    {
    }

    Protocol::PartMetaData preparePart(const QByteArray &part);

    Collection mCollection;
    Item mItem;
    QSet<QByteArray> mParts;
    Item::Id mUid;
    QDateTime mDatetime;
    QByteArray mPendingData;
    ItemCreateJob::MergeOptions mMergeOptions;
    bool mItemReceived;
};

Protocol::PartMetaData ItemCreateJobPrivate::preparePart(const QByteArray &partName)
{
    if (!mParts.remove(partName)) {
        // ERROR?
        return Protocol::PartMetaData();
    }

    mPendingData.clear();
    int version = 0;
    ItemSerializer::serialize(mItem, partName, mPendingData, version);

    return Protocol::PartMetaData(partName, mPendingData.size(), version);
}

ItemCreateJob::ItemCreateJob(const Item &item, const Collection &collection, QObject *parent)
    : Job(new ItemCreateJobPrivate(this), parent)
{
    Q_D(ItemCreateJob);

    Q_ASSERT(!item.mimeType().isEmpty());
    d->mItem = item;
    d->mParts = d->mItem.loadedPayloadParts();
    d->mCollection = collection;
}

ItemCreateJob::~ItemCreateJob()
{
}

void ItemCreateJob::doStart()
{
    Q_D(ItemCreateJob);

    Protocol::CreateItemCommand cmd;
    cmd.setMimeType(d->mItem.mimeType());
    cmd.setGID(d->mItem.gid());
    cmd.setRemoteId(d->mItem.remoteId());
    cmd.setRemoteRevision(d->mItem.remoteRevision());

    Protocol::CreateItemCommand::MergeModes mergeModes = Protocol::CreateItemCommand::None;
    if ((d->mMergeOptions & GID) && !d->mItem.gid().isEmpty()) {
        mergeModes |= Protocol::CreateItemCommand::GID;
    }
    if ((d->mMergeOptions & RID) && !d->mItem.remoteId().isEmpty()) {
        mergeModes |= Protocol::CreateItemCommand::RemoteID;
    }
    if ((d->mMergeOptions & Silent)) {
        mergeModes |= Protocol::CreateItemCommand::Silent;
    }
    const bool merge = (mergeModes & Protocol::CreateItemCommand::GID)
                        || (mergeModes & Protocol::CreateItemCommand::RemoteID);
    cmd.setMergeModes(mergeModes);


    if (d->mItem.d_func()->mFlagsOverwritten || !merge) {
        cmd.setFlags(d->mItem.flags());
    } else {
        cmd.setAddedFlags(d->mItem.d_func()->mAddedFlags);
        cmd.setRemovedFlags(d->mItem.d_func()->mDeletedFlags);
    }
    if (d->mItem.d_func()->mTagsOverwritten || !merge) {
        cmd.setTags(ProtocolHelper::entitySetToScope(d->mItem.d_func()->mAddedTags));
    } else {
        cmd.setAddedTags(ProtocolHelper::entitySetToScope(d->mItem.d_func()->mAddedTags));
        cmd.setRemovedTags(ProtocolHelper::entitySetToScope(d->mItem.d_func()->mDeletedTags));
    }

    cmd.setCollection(ProtocolHelper::entityToScope(d->mCollection));
    cmd.setItemSize(d->mItem.size());

    cmd.setAttributes(ProtocolHelper::attributesToProtocol(d->mItem));
    cmd.setParts(d->mParts);

    d->sendCommand(cmd);
}

void ItemCreateJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
{
    Q_D(ItemCreateJob);

    if (!response.isResponse() && response.type() == Protocol::Command::StreamPayload) {
        const Protocol::StreamPayloadCommand streamCmd(response);
        Protocol::StreamPayloadResponse streamResp;
        streamResp.setPayloadName(streamCmd.payloadName());
        if (streamCmd.request() == Protocol::StreamPayloadCommand::MetaData) {
            streamResp.setMetaData(d->preparePart(streamCmd.payloadName()));
        } else {
            if (streamCmd.destination().isEmpty()) {
                streamResp.setData(d->mPendingData);
            } else {
                QByteArray error;
                if (!ProtocolHelper::streamPayloadToFile(streamCmd.destination(), d->mPendingData, error)) {
                    // Error?
                }
            }
        }
        d->sendCommand(tag, streamResp);
    } else if (response.isResponse() && response.type() == Protocol::Command::FetchItems) {
        Protocol::FetchItemsResponse fetchResp(response);
        Item item = ProtocolHelper::parseItemFetchResult(fetchResp);
        if (!item.isValid()) {
            // Error, maybe?
            return;
        }
        d->mItemReceived = true;
        d->mItem = item;
    } else if (response.isResponse() && response.type() == Protocol::Command::CreateItem) {
        Protocol::CreateItemResponse createResp(response);
        emitResult();
    } else {
        Job::doHandleResponse(tag, response);
    }
}

void ItemCreateJob::setMerge(ItemCreateJob::MergeOptions options)
{
    Q_D(ItemCreateJob);

    d->mMergeOptions = options;
}

Item ItemCreateJob::item() const
{
    Q_D(const ItemCreateJob);

    if (d->mItemReceived) {
        return d->mItem;
    }

    if (d->mUid == 0) {
        return Item();
    }

    Item item(d->mItem);
    item.setId(d->mUid);
    item.setRevision(0);
    item.setModificationTime(d->mDatetime);
    item.setParentCollection(d->mCollection);
    item.setStorageCollectionId(d->mCollection.id());

    return item;
}
