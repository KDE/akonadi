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
#include "imapstreamparser.h"
#include "response.h"

#include <libs/imapparser_p.h>
#include <libs/protocol_p.h>
#include <shared/akstandarddirs.h>

#include <unistd.h>

#include <QFile>
#include <QFileInfo>

using namespace Akonadi;
using namespace Akonadi::Server;

PartStreamer::PartStreamer(Connection *connection, ImapStreamParser *parser,
                           const PimItem &pimItem, QObject *parent)
    : QObject(parent)
    , mConnection(connection)
    , mStreamParser(parser)
    , mItem(pimItem)
{
    // Make sure the file_db_data path exists
    AkStandardDirs::saveDir( "data", QLatin1String( "file_db_data" ) );
}

PartStreamer::~PartStreamer()
{
}

QByteArray PartStreamer::error() const
{
    return mError;
}

bool PartStreamer::streamNonliteral(Part& part, qint64& partSize, QByteArray& value)
{
    value = mStreamParser->readString();
    if (part.partType().ns() == QLatin1String("PLD")) {
        partSize = value.size();
    }
    if (!mDataChanged) {
        mDataChanged = (value.size() != part.datasize());
    }

    // only relevant for non-literals or non-external literals
    // only fallback to data comparision if part already exists and sizes match
    if (!mDataChanged) {
        mDataChanged = (value != PartHelper::translateData(part));
    }

    if (mDataChanged) {
        if (part.isValid()) {
            PartHelper::update(&part, value, value.size());
        } else {
//           akDebug() << "insert from Store::handleLine: " << value.left(100);
            part.setData(value);
            part.setDatasize(value.size());
            if (!PartHelper::insert(&part)) {
                mError = "Unable to add item part";
                return false;
            }
        }
    }

    return true;
}

bool PartStreamer::streamLiteral(Part& part, qint64& partSize, QByteArray& value)
{
    const qint64 dataSize = mStreamParser->remainingLiteralSize();
    if (part.partType().ns() == QLatin1String("PLD")) {
        partSize = dataSize;
        // Shortcut: if sizes differ, we don't need to compare data later no in order
        // to detect whether the part has changed
        if (!mDataChanged) {
            mDataChanged = (partSize != part.datasize());
        }
    }

    //actual case when streaming storage is used: external payload is enabled, data is big enough in a literal
    const bool storeInFile = dataSize > DbConfig::configuredDatabase()->sizeThreshold();
    if (storeInFile) {
        return streamLiteralToFile(dataSize, part, value);
    } else {
        mStreamParser->sendContinuationResponse(dataSize);
        //don't write in streaming way as the data goes to the database
        while (!mStreamParser->atLiteralEnd()) {
            value += mStreamParser->readLiteralPart();
        }
        if (part.isValid()) {
            PartHelper::update(&part, value, value.size());
        } else {
            part.setData(value);
            part.setDatasize(value.size());
            if (!part.insert()) {
              mError = "Failed to insert part to database";
              return false;
            }
        }
    }

    return true;
}


bool PartStreamer::streamLiteralToFile(qint64 dataSize, Part &part, QByteArray &value)
{
    const bool directStreaming = mConnection->capabilities().directStreaming();

    QByteArray origData;
    if (!mDataChanged && mCheckChanged) {
        origData = PartHelper::translateData(part);
    }

    if (directStreaming) {
        bool ok = streamLiteralToFileDirectly(dataSize, part);
        if (!ok) {
            return false;
        }
    } else {
        mStreamParser->sendContinuationResponse(dataSize);

        // use first part as value for the initial insert into / update to the database.
        // this will give us a proper filename to stream the rest of the parts contents into
        // NOTE: we have to set the correct size (== dataSize) directly
        value = mStreamParser->readLiteralPart();
        // akDebug() << Q_FUNC_INFO << "VALUE in STORE: " << value << value.size() << dataSize;

        if (part.isValid()) {
            PartHelper::update(&part, value, dataSize);
        } else {
            part.setData( value );
            part.setDatasize( dataSize );
            if ( !PartHelper::insert( &part ) ) {
                mError = "Unable to add item part";
                return false;
            }
        }

        //the actual streaming code for the remaining parts:
        // reads from the parser, writes immediately to the file
        QFile partFile(PartHelper::resolveAbsolutePath(part.data()));
        try {
            PartHelper::streamToFile(mStreamParser, partFile, QIODevice::WriteOnly | QIODevice::Append);
        } catch (const PartHelperException &e) {
            mError = e.what();
            return false;
        }
    }

    if (mCheckChanged && !mDataChanged) {
        // This is invoked only when part already exists, data sizes match and
        // caller wants to know whether parts really differ
        mDataChanged = (origData != PartHelper::translateData(part));
    }

    return true;
}

bool PartStreamer::streamLiteralToFileDirectly(qint64 dataSize, Part &part)
{
    QString filename;
    if (part.isValid()) {
        if (part.external()) {
            // Part was external and is still external
            filename = QString::fromLatin1(part.data());
        } else {
            // Part wasn't external, but is now
            filename = PartHelper::fileNameForPart(&part);
        }
        filename = PartHelper::updateFileNameRevision(filename);
    }


    QFileInfo finfo(filename);
    if (finfo.isAbsolute()) {
        filename = finfo.fileName();
    }

    part.setExternal(true);
    part.setDatasize(dataSize);
    part.setData(filename.toLatin1());

    if (part.isValid()) {
        if (!part.update()) {
            mError = "Failed to update part in database";
            return false;
        }
    } else {
        if (!part.insert()) {
            mError = "Failed to insert part into database";
            return false;
        }

        filename = PartHelper::updateFileNameRevision(PartHelper::fileNameForPart(&part));
        part.setData(filename.toLatin1());
        part.update();
    }

    Response response;
    response.setContinuation();
    response.setString("STREAM [FILE " + part.data() + "]");
    Q_EMIT responseAvailable(response);

    const QByteArray reply = mStreamParser->peekString();
    if (reply.startsWith("NO")) {
        akError() << "Client failed to store payload into file";
        mError = "Client failed to store payload into file";
        return false;
    }

    QFile file(PartHelper::resolveAbsolutePath(filename.toLatin1()), this);
    if (!file.exists()) {
        akError() << "External payload file does not exist";
        mError = "External payload file does not exist";
        return false;
    }

    if (file.size() != dataSize) {
        akError() << "Size mismatch! Client advertised" << dataSize << "bytes, but the file is" << file.size() << "bytes!";
        mError = "Payload size mismatch";
        return false;
    }

    return true;
}


bool PartStreamer::stream(const QByteArray &command, bool checkExists,
                          QByteArray &partName, qint64 &partSize, bool *changed)
{
    int partVersion = 0;
    partSize = 0;
    mError.clear();
    mDataChanged = false;
    mCheckChanged = (changed != 0);
    if (changed != 0) {
        *changed = false;
    }

    ImapParser::splitVersionedKey(command, partName, partVersion);
    const PartType partType = PartTypeHelper::fromFqName( partName );

    Part part;

    if (checkExists || mCheckChanged) {
        SelectQueryBuilder<Part> qb;
        qb.addValueCondition(Part::pimItemIdColumn(), Query::Equals, mItem.id());
        qb.addValueCondition(Part::partTypeIdColumn(), Query::Equals, partType.id());
        if (!qb.exec()) {
            mError = "Unable to check item part existence";
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

    // If the part is external, remember it's current file name
    QString originalFile;
    if (part.isValid() && part.external()) {
        originalFile = PartHelper::resolveAbsolutePath(part.data());
    }

    part.setPartType(partType);
    part.setVersion(partVersion);
    part.setPimItemId(mItem.id());

    QByteArray value;
    bool ok;
    if (mStreamParser->hasLiteral(false)) {
        ok = streamLiteral(part, partSize, value);
    } else {
        ok = streamNonliteral(part, partSize, value);
    }

    if (ok && mCheckChanged) {
        *changed = mDataChanged;
    }

    if (!originalFile.isEmpty()) {
        // If the part was external but is not anymore, or if it's still external
        // but the filename has changed (revision update), remove the original file
        if (!part.external() || (part.external() && originalFile != PartHelper::resolveAbsolutePath(part.data()))) {
            PartHelper::removeFile(originalFile);
        }
    }

    return ok;
}



