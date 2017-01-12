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
#include "private/protocol_p.h"
#include "helper_p.h"

#include <QDateTime>
#include <QFile>

#include <KLocalizedString>

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
    ProtocolHelper::PartNamespace ns; //dummy
    const QByteArray partLabel = ProtocolHelper::decodePartIdentifier(partName, ns);
    if (!mParts.remove(partLabel)) {
        // ERROR?
        return Protocol::PartMetaData();
    }

    mPendingData.clear();
    int version = 0;
    ItemSerializer::serialize(mItem, partLabel, mPendingData, version);

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

    if (!d->mCollection.isValid()) {
        setError(Unknown);
        setErrorText(i18n("Invalid parent collection"));
        emitResult();
        return;
    }

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

    if (d->mItem.d_ptr->mFlagsOverwritten || !merge) {
        cmd.setFlags(d->mItem.flags());
    } else {
        auto addedFlags = ItemChangeLog::instance()->addedFlags(d->mItem.d_ptr);
        auto deletedFlags = ItemChangeLog::instance()->deletedFlags(d->mItem.d_ptr);
        cmd.setAddedFlags(addedFlags);
        cmd.setRemovedFlags(deletedFlags);
    }
    auto addedTags = ItemChangeLog::instance()->addedTags(d->mItem.d_ptr);
    auto deletedTags = ItemChangeLog::instance()->deletedTags(d->mItem.d_ptr);
    if (!addedTags.isEmpty() && (d->mItem.d_ptr->mTagsOverwritten || !merge)) {
        cmd.setTags(ProtocolHelper::entitySetToScope(addedTags));
    } else {
        if (!addedTags.isEmpty()) {
            cmd.setAddedTags(ProtocolHelper::entitySetToScope(addedTags));
        }
        if (!deletedTags.isEmpty()) {
            cmd.setRemovedTags(ProtocolHelper::entitySetToScope(deletedTags));
        }
    }

    cmd.setCollection(ProtocolHelper::entityToScope(d->mCollection));
    cmd.setItemSize(d->mItem.size());

    cmd.setAttributes(ProtocolHelper::attributesToProtocol(d->mItem));
    QSet<QByteArray> parts;
    parts.reserve(d->mParts.size());
    for (const QByteArray &part : qAsConst(d->mParts)) {
        parts.insert(ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartPayload, part));
    }
    cmd.setParts(parts);

    d->sendCommand(cmd);
}

bool ItemCreateJob::doHandleResponse(qint64 tag, const Protocol::Command &response)
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
        return false;
    }

    if (response.isResponse() && response.type() == Protocol::Command::FetchItems) {
        Protocol::FetchItemsResponse fetchResp(response);
        Item item = ProtocolHelper::parseItemFetchResult(fetchResp);
        if (!item.isValid()) {
            // Error, maybe?
            return false;
        }
        d->mItem = item;
        return false;
    }

    if (response.isResponse() && response.type() == Protocol::Command::CreateItem) {
        return true;
    }

    return Job::doHandleResponse(tag, response);
}

void ItemCreateJob::setMerge(ItemCreateJob::MergeOptions options)
{
    Q_D(ItemCreateJob);

    d->mMergeOptions = options;
}

Item ItemCreateJob::item() const
{
    Q_D(const ItemCreateJob);

    // Parent collection is available only with non-silent merge/create
    if (d->mItem.parentCollection().isValid()) {
        return d->mItem;
    }

    Item item(d->mItem);
    item.setRevision(0);
    item.setModificationTime(d->mDatetime);
    item.setParentCollection(d->mCollection);
    item.setStorageCollectionId(d->mCollection.id());

    return item;
}
