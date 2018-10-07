/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "partstreamer.h"
#include "parthelper.h"
#include "parttypehelper.h"
#include "selectquerybuilder.h"
#include "dbconfig.h"
#include "connection.h"
#include "capabilities_p.h"
#include "akonadiserver_debug.h"

#include <private/protocol_p.h>
#include <private/standarddirs_p.h>
#include <private/externalpartstorage_p.h>

#include <config-akonadi.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <QFile>
#include <QFileInfo>

using namespace Akonadi;
using namespace Akonadi::Server;

PartStreamer::PartStreamer(Connection *connection,
                           const PimItem &pimItem)
    : mConnection(connection)
    , mItem(pimItem)
{
    // Make sure the file_db_data path exists
    StandardDirs::saveDir("data", QStringLiteral("file_db_data"));
}

PartStreamer::~PartStreamer()
{
}

QString PartStreamer::error() const
{
    return mError;
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
        mError = QStringLiteral("Client failed to provide part metadata");
        return Protocol::PartMetaData();
    }

    return Protocol::cmdCast<Protocol::StreamPayloadResponse>(cmd).metaData();
}

bool PartStreamer::streamPayload(Part &part, const QByteArray &partName)
{
    Protocol::PartMetaData metaPart = requestPartMetaData(partName);
    if (metaPart.name().isEmpty()) {
        mError = QStringLiteral("Part name is empty");
        return false;
    }
    part.setVersion(metaPart.version());

    if (part.datasize() != metaPart.size()) {
        part.setDatasize(metaPart.size());
        // Shortcut: if sizes differ, we don't need to compare data later no in order
        // to detect whether the part has changed
        mDataChanged = mDataChanged || (metaPart.size() != part.datasize());
    }

    if (metaPart.storageType() == Protocol::PartMetaData::Foreign) {
        return streamForeignPayload(part, metaPart);
    } else if (part.datasize() > DbConfig::configuredDatabase()->sizeThreshold()) {
        //actual case when streaming storage is used: external payload is enabled,
        // data is big enough in a literal
        return streamPayloadToFile(part, metaPart);
    } else {
        return streamPayloadData(part, metaPart);
    }
}

bool PartStreamer::streamPayloadData(Part &part, const Protocol::PartMetaData &metaPart)
{
    // If the part WAS external previously, remove data file
    if (part.storage() == Part::External) {
        ExternalPartStorage::self()->removePartFile(
            ExternalPartStorage::resolveAbsolutePath(part.data()));
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
        mError = QStringLiteral("Client failed to provide payload data");
        qCCritical(AKONADISERVER_LOG) << mError;
        return false;
    }
    const QByteArray newData = response.data();
    // only use the data size with internal payload parts, for foreign parts
    // we use the size reported by client
    const auto newSize = (metaPart.storageType() == Protocol::PartMetaData::Internal)
                                ? newData.size()
                                : metaPart.size();
    if (newSize != metaPart.size()) {
        mError = QStringLiteral("Payload size mismatch");
        return false;
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
            mError = QStringLiteral("Failed to insert part to database");
            return false;
        }
    }

    return true;
}

bool PartStreamer::streamPayloadToFile(Part &part, const Protocol::PartMetaData &metaPart)
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
                ExternalPartStorage::self()->removePartFile(
                    ExternalPartStorage::resolveAbsolutePath(filename));
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
            mError = QStringLiteral("Failed to update part in database");
            return false;
        }
    } else {
        if (!part.insert()) {
            mError = QStringLiteral("Failed to insert part into database");
            return false;
        }

        filename = ExternalPartStorage::nameForPartId(part.id());
        part.setData(filename);
        if (!part.update()) {
            mError = QStringLiteral("Failed to update part in database");
            return false;
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
        mError = QStringLiteral("Client failed to store payload into file");
        qCCritical(AKONADISERVER_LOG) << mError;
        return false;
    }

    QFile file(ExternalPartStorage::resolveAbsolutePath(filename));
    if (!file.exists()) {
        mError = QStringLiteral("External payload file does not exist");
        qCCritical(AKONADISERVER_LOG) << mError;
        return false;
    }

    if (file.size() != metaPart.size()) {
        mError = QStringLiteral("Payload size mismatch");
        qCDebug(AKONADISERVER_LOG) << mError << ", client advertised" << metaPart.size() << "bytes, but the file is" << file.size() << "bytes!";
        return false;
    }

    if (mCheckChanged && !mDataChanged) {
        // This is invoked only when part already exists, data sizes match and
        // caller wants to know whether parts really differ
        mDataChanged = (origData != PartHelper::translateData(part));
    }

    return true;
}

bool PartStreamer::streamForeignPayload(Part &part, const Protocol::PartMetaData &metaPart)
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
    const auto response = Protocol::cmdCast<Protocol::StreamPayloadResponse>(cmd);
    if (!response.isValid() || response.isError()) {
        mError = QStringLiteral("Client failed to store payload into file");
        qCCritical(AKONADISERVER_LOG) << mError;
        return false;
    }

    // If the part was previously external, clean up the data
    if (part.storage() == Part::External) {
        const QString filename = QString::fromUtf8(part.data());
        ExternalPartStorage::self()->removePartFile(
                ExternalPartStorage::resolveAbsolutePath(filename));
    }

    part.setStorage(Part::Foreign);
    part.setData(response.data());

    if (part.isValid()) {
        if (!part.update()) {
            mError = QStringLiteral("Failed to update part in database");
            return false;
        }
    } else {
        if (!part.insert()) {
            mError = QStringLiteral("Failed to insert part into database");
            return false;
        }
    }

    const QString filename = QString::fromUtf8(response.data());
    QFile file(filename);
    if (!file.exists()) {
        mError = QStringLiteral("Foreign payload file does not exist");
        qCCritical(AKONADISERVER_LOG) << mError;
        return false;
    }

    if (file.size() != metaPart.size()) {
        mError = QStringLiteral("Payload size mismatch");
        qCCritical(AKONADISERVER_LOG) << mError << ", client advertised" << metaPart.size() << "bytes, but the file size is" << file.size() << "bytes!";
        return false;
    }

    if (mCheckChanged && !mDataChanged) {
        // This is invoked only when part already exists, data sizes match and
        // caller wants to know whether parts really differ
        mDataChanged = (origData != PartHelper::translateData(part));
    }

    return true;
}

bool PartStreamer::preparePart(bool checkExists, const QByteArray &partName, Part &part)
{
    mError.clear();
    mDataChanged = false;

    const PartType partType = PartTypeHelper::fromFqName(partName);

    if (checkExists || mCheckChanged) {
        SelectQueryBuilder<Part> qb;
        qb.addValueCondition(Part::pimItemIdColumn(), Query::Equals, mItem.id());
        qb.addValueCondition(Part::partTypeIdColumn(), Query::Equals, partType.id());
        if (!qb.exec()) {
            mError = QStringLiteral("Unable to check item part existence");
            return false;
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

    return true;
}

bool PartStreamer::stream(bool checkExists, const QByteArray &partName, qint64 &partSize, bool *changed)
{
    mCheckChanged = (changed != nullptr);
    if (changed != nullptr) {
        *changed = false;
    }

    Part part;
    if (!preparePart(checkExists, partName, part)) {
        return false;
    }

    bool ok = streamPayload(part, partName);
    if (changed && ok && mCheckChanged) {
        *changed = mDataChanged;
    }

    partSize = part.datasize();

    return ok;
}

bool PartStreamer::streamAttribute(bool checkExists, const QByteArray &_partName, const QByteArray &value, bool *changed)
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
    if (!preparePart(checkExists, partName, part)) {
        return false;
    }

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
                mError = QStringLiteral("Failed to store part in database");
                return false;
            }
            PartHelper::update(&part, value, value.size());
        } else {
            part.setData(value);
            if (!part.insert()) {
                mError = QStringLiteral("Failed to store part in database");
                return false;
            }
        }
    }

    if (mCheckChanged) {
        *changed = mDataChanged;
    }

    return true;
}

