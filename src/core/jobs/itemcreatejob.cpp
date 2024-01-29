/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2007 Robert Zwerus <arzie@dds.nl>
    SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemcreatejob.h"

#include "collection.h"
#include "gidextractor_p.h"
#include "item.h"
#include "item_p.h"
#include "itemserializer_p.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

#include <QFile>

#include <KLocalizedString>

using namespace Akonadi;

class Akonadi::ItemCreateJobPrivate : public JobPrivate
{
public:
    explicit ItemCreateJobPrivate(ItemCreateJob *parent)
        : JobPrivate(parent)
    {
    }

    Protocol::PartMetaData preparePart(const QByteArray &part);

    QString jobDebuggingString() const override;
    Collection mCollection;
    Item mItem;
    QSet<QByteArray> mParts;
    QSet<QByteArray> mForeignParts;
    struct PendingPart {
        void clear()
        {
            name.clear();
            data.clear();
        }

        QByteArray name;
        QByteArray data;
    } mPendingPart;
    ItemCreateJob::MergeOptions mMergeOptions = ItemCreateJob::NoMerge;
    bool mItemReceived = false;
};

QString Akonadi::ItemCreateJobPrivate::jobDebuggingString() const
{
    const QString collectionName = mCollection.name();
    QString str = QStringLiteral("%1 Item %2 from col %3")
                      .arg(mMergeOptions == ItemCreateJob::NoMerge ? QStringLiteral("Create") : QStringLiteral("Merge"))
                      .arg(mItem.id())
                      .arg(mCollection.id());
    if (!collectionName.isEmpty()) {
        str += QStringLiteral(" (%1)").arg(collectionName);
    }
    return str;
}

Protocol::PartMetaData ItemCreateJobPrivate::preparePart(const QByteArray &partName)
{
    ProtocolHelper::PartNamespace ns; // dummy
    const QByteArray partLabel = ProtocolHelper::decodePartIdentifier(partName, ns);
    if (!mParts.remove(partLabel)) {
        // ERROR?
        return Protocol::PartMetaData();
    }

    int version = 0;
    if (mForeignParts.contains(partLabel)) {
        mPendingPart = PendingPart{.name = partName, .data = mItem.d_ptr->mPayloadPath.toUtf8()};
        const auto size = QFile(mItem.d_ptr->mPayloadPath).size();
        return Protocol::PartMetaData(partName, size, version, Protocol::PartMetaData::Foreign);
    } else {
        mPendingPart.clear();
        mPendingPart.name = partName;
        ItemSerializer::serialize(mItem, partLabel, mPendingPart.data, version);
        return Protocol::PartMetaData(partName, mPendingPart.data.size(), version);
    }
}

ItemCreateJob::ItemCreateJob(const Item &item, const Collection &collection, QObject *parent)
    : Job(new ItemCreateJobPrivate(this), parent)
{
    Q_D(ItemCreateJob);

    Q_ASSERT(!item.mimeType().isEmpty());
    d->mItem = item;
    d->mParts = d->mItem.loadedPayloadParts();
    d->mCollection = collection;

    if (!d->mItem.payloadPath().isEmpty()) {
        d->mForeignParts = ItemSerializer::allowedForeignParts(d->mItem);
    }
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

    auto cmd = Protocol::CreateItemCommandPtr::create();
    cmd->setMimeType(d->mItem.mimeType());
    cmd->setGid(d->mItem.gid());
    cmd->setRemoteId(d->mItem.remoteId());
    cmd->setRemoteRevision(d->mItem.remoteRevision());
    cmd->setModificationTime(d->mItem.modificationTime());

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
    const bool merge = (mergeModes & Protocol::CreateItemCommand::GID) || (mergeModes & Protocol::CreateItemCommand::RemoteID);
    cmd->setMergeModes(mergeModes);

    if (d->mItem.d_ptr->mFlagsOverwritten || !merge) {
        cmd->setFlags(d->mItem.flags());
        cmd->setFlagsOverwritten(d->mItem.d_ptr->mFlagsOverwritten);
    } else {
        const auto addedFlags = ItemChangeLog::instance()->addedFlags(d->mItem.d_ptr);
        const auto deletedFlags = ItemChangeLog::instance()->deletedFlags(d->mItem.d_ptr);
        cmd->setAddedFlags(addedFlags);
        cmd->setRemovedFlags(deletedFlags);
    }

    if (d->mItem.d_ptr->mTagsOverwritten || !merge) {
        const auto tags = d->mItem.tags();
        if (!tags.isEmpty()) {
            cmd->setTags(ProtocolHelper::entitySetToScope(tags));
        }
    } else {
        const auto addedTags = ItemChangeLog::instance()->addedTags(d->mItem.d_ptr);
        if (!addedTags.isEmpty()) {
            cmd->setAddedTags(ProtocolHelper::entitySetToScope(addedTags));
        }
        const auto deletedTags = ItemChangeLog::instance()->deletedTags(d->mItem.d_ptr);
        if (!deletedTags.isEmpty()) {
            cmd->setRemovedTags(ProtocolHelper::entitySetToScope(deletedTags));
        }
    }

    cmd->setCollection(ProtocolHelper::entityToScope(d->mCollection));
    cmd->setItemSize(d->mItem.size());

    cmd->setAttributes(ProtocolHelper::attributesToProtocol(d->mItem));
    QSet<QByteArray> parts;
    parts.reserve(d->mParts.size());
    for (const QByteArray &part : std::as_const(d->mParts)) {
        parts.insert(ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartPayload, part));
    }
    cmd->setParts(parts);

    d->sendCommand(cmd);
}

bool ItemCreateJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(ItemCreateJob);

    if (!response->isResponse() && response->type() == Protocol::Command::StreamPayload) {
        const auto &streamCmd = Protocol::cmdCast<Protocol::StreamPayloadCommand>(response);
        auto streamResp = Protocol::StreamPayloadResponsePtr::create();
        streamResp->setPayloadName(streamCmd.payloadName());
        if (streamCmd.request() == Protocol::StreamPayloadCommand::MetaData) {
            streamResp->setMetaData(d->preparePart(streamCmd.payloadName()));
        } else if (streamCmd.request() == Protocol::StreamPayloadCommand::Data) {
            if (streamCmd.payloadName() != d->mPendingPart.name) {
                streamResp->setError(1, QStringLiteral("Unexpected payload name"));
            } else if (streamCmd.destination().isEmpty()) {
                streamResp->setData(d->mPendingPart.data);
            } else {
                QByteArray error;
                if (!ProtocolHelper::streamPayloadToFile(streamCmd.destination(), d->mPendingPart.data, error)) {
                    streamResp->setError(1, QStringLiteral("Failed to stream payload to file: %1").arg(QString::fromUtf8(error)));
                }
            }
        } else {
            streamResp->setError(1, QStringLiteral("Unknown stream payload request"));
        }
        d->sendCommand(tag, streamResp);
        return false;
    }

    if (response->isResponse() && response->type() == Protocol::Command::FetchItems) {
        const auto &fetchResp = Protocol::cmdCast<Protocol::FetchItemsResponse>(response);
        Item item = ProtocolHelper::parseItemFetchResult(fetchResp);
        if (!item.isValid()) {
            // Error, maybe?
            return false;
        }
        d->mItem = item;
        return false;
    }

    if (response->isResponse() && response->type() == Protocol::Command::CreateItem) {
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
    item.setParentCollection(d->mCollection);
    item.setStorageCollectionId(d->mCollection.id());

    return item;
}

#include "moc_itemcreatejob.cpp"
