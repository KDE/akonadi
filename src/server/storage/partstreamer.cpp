/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "partstreamer.h"
#include "akonadiserver_debug.h"
#include "capabilities_p.h"
#include "connection.h"
#include "dbconfig.h"
#include "parthelper.h"
#include "parttypehelper.h"
#include "selectquerybuilder.h"

#include "private/externalpartstorage_p.h"
#include "private/protocol_p.h"
#include "private/standarddirs_p.h"

#include <config-akonadi.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <QFile>
#include <QFileInfo>

using namespace Akonadi;
using namespace Akonadi::Server;

PartStreamer::PartStreamer(Connection *connection, const PimItem &pimItem)
    : mConnection(connection)
    , mItem(pimItem)
{
    // Make sure the file_db_data path exists
    StandardDirs::saveDir("data", QStringLiteral("file_db_data"));
}

PartStreamer::~PartStreamer()
{
}

Protocol::PartMetaData PartStreamer::requestPartMetaData(const QByteArray &partName)
{
    {
        Protocol::StreamPayloadCommand resp;
        resp.setPayloadName(partName);
        resp.setRequest(Protocol::StreamPayloadCommand::MetaData);
        mConnection->sendResponse(std::move(resp));
    }

    const auto cmd = mConnection->readCommand();
    if (!cmd->isValid() || Protocol::cmdCast<Protocol::Response>(cmd).isError()) {
        throw PartStreamerException("Client failed to provide part metadata.");
    }

    return Protocol::cmdCast<Protocol::StreamPayloadResponse>(cmd).metaData();
}

void PartStreamer::streamPayload(Part &part, const QByteArray &partName)
{
    Protocol::PartMetaData metaPart = requestPartMetaData(partName);
    if (metaPart.name().isEmpty()) {
        throw PartStreamerException(QStringLiteral("Client sent empty metadata for part '%1'.").arg(QString::fromUtf8(partName)));
    }
    if (metaPart.name() != partName) {
        throw PartStreamerException(
            QStringLiteral("Client sent metadata for part '%1' but requested part '%2'.").arg(QString::fromUtf8(metaPart.name()), QString::fromUtf8(partName)));
    }
    part.setVersion(metaPart.version());

    if (part.datasize() != metaPart.size()) {
        part.setDatasize(metaPart.size());
        // Shortcut: if sizes differ, we don't need to compare data later no in order
        // to detect whether the part has changed
        mDataChanged = mDataChanged || (metaPart.size() != part.datasize());
    }

    if (metaPart.storageType() == Protocol::PartMetaData::Foreign) {
        streamForeignPayload(part, metaPart);
    } else if (part.datasize() > DbConfig::configuredDatabase()->sizeThreshold()) {
        // actual case when streaming storage is used: external payload is enabled,
        // data is big enough in a literal
        streamPayloadToFile(part, metaPart);
    } else {
        streamPayloadData(part, metaPart);
    }
}

void PartStreamer::streamPayloadData(Part &part, const Protocol::PartMetaData &metaPart)
{
    // If the part WAS external previously, remove data file
    if (part.storage() == Part::External) {
        ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath(part.data()));
    }

    // Request the actual data
    {
        Protocol::StreamPayloadCommand resp;
        resp.setPayloadName(metaPart.name());
        resp.setRequest(Protocol::StreamPayloadCommand::Data);
        mConnection->sendResponse(std::move(resp));
    }

    const auto cmd = mConnection->readCommand();
    const auto &response = Protocol::cmdCast<Protocol::StreamPayloadResponse>(cmd);
    if (!response.isValid() || response.isError()) {
        throw PartStreamerException(QStringLiteral("Client failed to provide payload data for part ID %1 (%2).").arg(part.id()).arg(part.partType().name()));
    }
    const QByteArray newData = response.data();
    // only use the data size with internal payload parts, for foreign parts
    // we use the size reported by client
    const auto newSize = (metaPart.storageType() == Protocol::PartMetaData::Internal) ? newData.size() : metaPart.size();
    if (newSize != metaPart.size()) {
        throw PartStreamerException(QStringLiteral("Payload size mismatch: client advertised %1 bytes but sent %2 bytes.").arg(metaPart.size()).arg(newSize));
    }

    if (part.isValid()) {
        if (!mDataChanged) {
            mDataChanged = mDataChanged || (newData != part.data());
        }
        PartHelper::update(&part, newData, newSize);
    } else {
        part.setData(newData);
        part.setDatasize(newSize);
        if (!part.insert()) {
            throw PartStreamerException("Failed to insert new part into database.");
        }
    }
}

void PartStreamer::streamPayloadToFile(Part &part, const Protocol::PartMetaData &metaPart)
{
    QByteArray origData;
    if (!mDataChanged && mCheckChanged) {
        origData = PartHelper::translateData(part);
    }

    QByteArray filename;
    if (part.isValid()) {
        if (part.storage() == Part::External) {
            // Part was external and is still external
            filename = part.data();
            if (!filename.isEmpty()) {
                ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath(filename));
                filename = ExternalPartStorage::updateFileNameRevision(filename);
            } else {
                // recover from data corruption
                filename = ExternalPartStorage::nameForPartId(part.id());
            }
        } else {
            // Part wasn't external, but is now
            filename = ExternalPartStorage::nameForPartId(part.id());
        }

        QFileInfo finfo(QString::fromUtf8(filename));
        if (finfo.isAbsolute()) {
            filename = finfo.fileName().toUtf8();
        }
    }

    part.setStorage(Part::External);
    part.setDatasize(metaPart.size());
    part.setData(filename);

    if (part.isValid()) {
        if (!part.update()) {
            throw PartStreamerException(QStringLiteral("Failed to update part %1 in database.").arg(part.id()));
        }
    } else {
        if (!part.insert()) {
            throw PartStreamerException(QStringLiteral("Failed to insert new part fo PimItem %1 into database.").arg(part.pimItemId()));
        }

        filename = ExternalPartStorage::nameForPartId(part.id());
        part.setData(filename);
        if (!part.update()) {
            throw PartStreamerException(QStringLiteral("Failed to update part %1 in database.").arg(part.id()));
        }
    }

    {
        Protocol::StreamPayloadCommand cmd;
        cmd.setPayloadName(metaPart.name());
        cmd.setRequest(Protocol::StreamPayloadCommand::Data);
        cmd.setDestination(QString::fromUtf8(filename));
        mConnection->sendResponse(std::move(cmd));
    }

    const auto cmd = mConnection->readCommand();
    const auto &response = Protocol::cmdCast<Protocol::Response>(cmd);
    if (!response.isValid() || response.isError()) {
        throw PartStreamerException("Client failed to store payload into file.");
    }

    QFile file(ExternalPartStorage::resolveAbsolutePath(filename));
    if (!file.exists()) {
        throw PartStreamerException(QStringLiteral("External payload file %1 does not exist.").arg(file.fileName()));
    }

    if (file.size() != metaPart.size()) {
        throw PartStreamerException(
            QStringLiteral("Payload size mismatch, client advertised %1 bytes, but the file is %2 bytes.").arg(metaPart.size(), file.size()));
    }

    if (mCheckChanged && !mDataChanged) {
        // This is invoked only when part already exists, data sizes match and
        // caller wants to know whether parts really differ
        mDataChanged = (origData != PartHelper::translateData(part));
    }
}

void PartStreamer::streamForeignPayload(Part &part, const Protocol::PartMetaData &metaPart)
{
    QByteArray origData;
    if (!mDataChanged && mCheckChanged) {
        origData = PartHelper::translateData(part);
    }

    {
        Protocol::StreamPayloadCommand cmd;
        cmd.setPayloadName(metaPart.name());
        cmd.setRequest(Protocol::StreamPayloadCommand::Data);
        mConnection->sendResponse(std::move(cmd));
    }

    const auto cmd = mConnection->readCommand();
    const auto &response = Protocol::cmdCast<Protocol::StreamPayloadResponse>(cmd);
    if (!response.isValid() || response.isError()) {
        throw PartStreamerException("Client failed to store payload into file.");
    }

    // If the part was previously external, clean up the data
    if (part.storage() == Part::External) {
        const QString filename = QString::fromUtf8(part.data());
        ExternalPartStorage::self()->removePartFile(ExternalPartStorage::resolveAbsolutePath(filename));
    }

    part.setStorage(Part::Foreign);
    part.setData(response.data());

    if (part.isValid()) {
        if (!part.update()) {
            throw PartStreamerException(QStringLiteral("Failed to update part %1 in database.").arg(part.id()));
        }
    } else {
        if (!part.insert()) {
            throw PartStreamerException(QStringLiteral("Failed to insert part for PimItem %1 into database.").arg(part.pimItemId()));
        }
    }

    const QString filename = QString::fromUtf8(response.data());
    QFile file(filename);
    if (!file.exists()) {
        throw PartStreamerException(QStringLiteral("Foreign payload file %1 does not exist.").arg(filename));
    }

    if (file.size() != metaPart.size()) {
        throw PartStreamerException(
            QStringLiteral("Foreign payload size mismatch, client advertised %1 bytes, but the file size is %2 bytes.").arg(metaPart.size(), file.size()));
    }

    if (mCheckChanged && !mDataChanged) {
        // This is invoked only when part already exists, data sizes match and
        // caller wants to know whether parts really differ
        mDataChanged = (origData != PartHelper::translateData(part));
    }
}

void PartStreamer::preparePart(bool checkExists, const QByteArray &partName, Part &part)
{
    mDataChanged = false;

    const PartType partType = PartTypeHelper::fromFqName(partName);

    if (checkExists || mCheckChanged) {
        SelectQueryBuilder<Part> qb;
        qb.addValueCondition(Part::pimItemIdColumn(), Query::Equals, mItem.id());
        qb.addValueCondition(Part::partTypeIdColumn(), Query::Equals, partType.id());
        if (!qb.exec()) {
            throw PartStreamerException(QStringLiteral("Failed to check if part %1 exists in PimItem %2.").arg(QString::fromUtf8(partName)).arg(mItem.id()));
        }

        const Part::List result = qb.result();
        if (!result.isEmpty()) {
            part = result.at(0);
        }
    }

    // Shortcut: newly created parts are always "changed"
    if (!part.isValid()) {
        mDataChanged = true;
    }

    part.setPartType(partType);
    part.setPimItemId(mItem.id());
}

void PartStreamer::stream(bool checkExists, const QByteArray &partName, qint64 &partSize, bool *changed)
{
    mCheckChanged = (changed != nullptr);
    if (changed != nullptr) {
        *changed = false;
    }

    Part part;
    preparePart(checkExists, partName, part);

    streamPayload(part, partName);
    if (changed && mCheckChanged) {
        *changed = mDataChanged;
    }

    partSize = part.datasize();
}

void PartStreamer::streamAttribute(bool checkExists, const QByteArray &_partName, const QByteArray &value, bool *changed)
{
    mCheckChanged = (changed != nullptr);
    if (changed != nullptr) {
        *changed = false;
    }

    QByteArray partName;
    if (!_partName.startsWith("ATR:")) {
        partName = "ATR:" + _partName;
    } else {
        partName = _partName;
    }

    Part part;
    preparePart(checkExists, partName, part);

    if (part.isValid()) {
        if (mCheckChanged) {
            if (PartHelper::translateData(part) != value) {
                mDataChanged = true;
            }
        }
        PartHelper::update(&part, value, value.size());
    } else {
        const bool storeInFile = value.size() > DbConfig::configuredDatabase()->sizeThreshold();
        part.setDatasize(value.size());
        part.setVersion(0);
        if (storeInFile) {
            if (!part.insert()) {
                throw PartStreamerException(QStringLiteral("Failed to store attribute part for PimItem %1 in database.").arg(part.pimItemId()));
            }
            PartHelper::update(&part, value, value.size());
        } else {
            part.setData(value);
            if (!part.insert()) {
                throw PartStreamerException(QStringLiteral("Failed to store attribute part for PimItem %1 in database.").arg(part.pimItemId()));
            }
        }
    }

    if (mCheckChanged) {
        *changed = mDataChanged;
    }
}
