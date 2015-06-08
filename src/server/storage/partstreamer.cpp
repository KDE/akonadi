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

#include <private/protocol_p.h>
#include <shared/akstandarddirs.h>

#include <config-akonadi.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <QFile>
#include <QFileInfo>

using namespace Akonadi;
using namespace Akonadi::Server;

PartStreamer::PartStreamer(Connection *connection,
                           const PimItem &pimItem, QObject *parent)
    : QObject(parent)
    , mConnection(connection)
    , mItem(pimItem)
{
    // Make sure the file_db_data path exists
    AkStandardDirs::saveDir("data", QStringLiteral("file_db_data"));
}

PartStreamer::~PartStreamer()
{
}

QString PartStreamer::error() const
{
    return mError;
}

bool PartStreamer::streamPayload(Part &part, Protocol::PartMetaData &metaPart)
{
    if (part.datasize() != metaPart.size()) {
        part.setDatasize(metaPart.size());
        // Shortcut: if sizes differ, we don't need to compare data later no in order
        // to detect whether the part has changed
        mDataChanged = mDataChanged || (metaPart.size() != part.datasize());
    }

    //actual case when streaming storage is used: external payload is enabled, data is big enough in a literal
    const bool storeInFile = part.datasize() > DbConfig::configuredDatabase()->sizeThreshold();
    if (storeInFile) {
        return streamPayloadToFile(part, metaPart);
    } else {
        Protocol::StreamPayloadCommand cmd;
        cmd.setExpectedSize(metaPart.size());
        cmd.setPayloadName(metaPart.name());
        mConnection->sendResponse(cmd);

        Protocol::StreamPayloadResponse response = mConnection->readCommand();
        if (response.isError()) {
            mError = QStringLiteral("Client failed to store payload into file");
            akError() << mError;
            return false;
        }
        const QByteArray newData = response.data();
        if (part.isValid()) {
            if (!mDataChanged) {
                mDataChanged = mDataChanged || (newData != part.data());
            }
            PartHelper::update(&part, newData, newData.size());
        } else {
            part.setData(newData);
            part.setDatasize(newData.size());
            if (!part.insert()) {
                mError = QStringLiteral("Failed to insert part to database");
                return false;
            }
        }
    }

    return true;
}

bool PartStreamer::streamPayloadToFile(Part &part, Protocol::PartMetaData &metaPart)
{
    QByteArray origData;
    if (!mDataChanged && mCheckChanged) {
        origData = PartHelper::translateData(part);
    }

    QString filename;
    if (part.isValid()) {
        if (part.external()) {
            // Part was external and is still external
            filename = QString::fromLatin1(part.data());
        } else {
            // Part wasn't external, but is now
            filename = PartHelper::fileNameForPart(&part);
        }
        // FIXME: This leaks the original part!
        filename = PartHelper::updateFileNameRevision(filename);
    }

    QFileInfo finfo(filename);
    if (finfo.isAbsolute()) {
        filename = finfo.fileName();
    }

    part.setExternal(true);
    part.setDatasize(metaPart.size());
    part.setData(filename.toLatin1());

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

        filename = PartHelper::updateFileNameRevision(PartHelper::fileNameForPart(&part));
        part.setData(filename.toLatin1());
        if (!part.update()) {
            mError = QStringLiteral("Failed to update part in database");
            return false;
        }
    }

    Protocol::StreamPayloadCommand cmd;
    cmd.setPayloadName(metaPart.name());
    cmd.setExpectedSize(metaPart.size());
    cmd.setExternalFile(filename);
    mConnection->sendResponse(cmd);

    Protocol::StreamPayloadResponse response = mConnection->readCommand();
    if (response.isError()) {
        mError = QStringLiteral("Client failed to store payload into file");
        akError() << mError;
        return false;
    }

    QFile file(PartHelper::resolveAbsolutePath(filename.toLatin1()), this);
    if (!file.exists()) {
        mError = QStringLiteral("External payload file does not exist");
        akError() << mError;
        return false;
    }

    if (file.size() != metaPart.size()) {
        mError = QStringLiteral("Payload size mismatch");
        akError() << mError << ", client advertised" << metaPart.size() << "bytes, but the file is" << file.size() << "bytes!";
        return false;
    }

    if (mCheckChanged && !mDataChanged) {
        // This is invoked only when part already exists, data sizes match and
        // caller wants to know whether parts really differ
        mDataChanged = (origData != PartHelper::translateData(part));
    }

    return true;
}

bool PartStreamer::stream(bool checkExists, Protocol::PartMetaData &metaPart, bool *changed)
{
    mError.clear();
    mDataChanged = false;
    mCheckChanged = (changed != 0);
    if (changed != 0) {
        *changed = false;
    }
    const PartType partType = PartTypeHelper::fromFqName(metaPart.name());

    Part part;

    if (checkExists || mCheckChanged) {
        SelectQueryBuilder<Part> qb;
        qb.addValueCondition(Part::pimItemIdColumn(), Query::Equals, mItem.id());
        qb.addValueCondition(Part::partTypeIdColumn(), Query::Equals, partType.id());
        if (!qb.exec()) {
            mError = QStringLiteral("Unable to check item part existence");
            return false;
        }

        Part::List result = qb.result();
        if (!result.isEmpty()) {
            part = result.first();
        }
    }

    // Shortcut: newly created parts are always "changed"
    if (!part.isValid()) {
        mDataChanged = true;
    }

    part.setPartType(partType);
    part.setVersion(metaPart.version());
    part.setPimItemId(mItem.id());

    bool ok = streamPayload(part, metaPart);
    if (ok && mCheckChanged) {
        *changed = mDataChanged;
    }

    return ok;
}
