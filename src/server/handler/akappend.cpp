/***************************************************************************
 *   Copyright (C) 2007 by Robert Zwerus <arzie@dds.nl>                    *
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

#include "akappend.h"

#include <private/imapparser_p.h>
#include <private/protocol_p.h>

#include "imapstreamparser.h"

#include "response.h"
#include "handlerhelper.h"

#include "akonadi.h"
#include "connection.h"
#include "preprocessormanager.h"
#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/transaction.h"
#include "storage/parttypehelper.h"
#include "storage/dbconfig.h"
#include "storage/partstreamer.h"
#include "storage/parthelper.h"

#include <QtCore/QDebug>

using namespace Akonadi;
using namespace Akonadi::Server;

AkAppend::AkAppend()
    : Handler()
{
}

AkAppend::~AkAppend()
{
}

QByteArray AkAppend::parseFlag(const QByteArray &flag) const
{
    const int pos1 = flag.indexOf('[');
    const int pos2 = flag.lastIndexOf(']');
    return flag.mid(pos1 + 1, pos2 - pos1 - 1);
}

bool AkAppend::buildPimItem(Protocol::CreateItemCommand &cmd, PimItem &item)
{
    MimeType mimeType = MimeType::retrieveByName(QString::fromLatin1(cmd.mimeType()));
    if (!mimeType.isValid()) {
        MimeType m(QString::fromLatin1(cmd.mimeType()));
        if (!m.insert()) {
            return failureResponse(QByteArray("Unable to create mimetype '") + cmd.mimeType() + QByteArray("'."));
        }
        mimeType = m;
    }

    const qint64 colId = HandlerHelper::collectionIdFromScope(cmd.collection());

    item.setRev(0);
    item.setSize(cmd.itemSize());
    item.setMimeTypeId(mimeType.id());
    item.setCollectionId(colId);
    item.setDatetime(cmd.dateTime());
    if (cmd.remoteId().isEmpty()) {
        // from application
        item.setDirty(true);
    } else {
        // from resource
        item.setRemoteId(cmd.remoteId());
        item.setDirty(false);
    }
    item.setRemoteRevision(cmd.remoteRevision());
    item.setGid(cmd.gid());
    item.setAtime(QDateTime::currentDateTime());

    return true;
}

// This is used for clients that don't support item streaming
bool AkAppend::readParts(PimItem &pimItem)
{

    // parse part specification
    QVector<QPair<QByteArray, QPair<qint64, int> > > partSpecs;
    QByteArray partName = "";
    qint64 partSize = -1;
    qint64 partSizes = 0;
    bool ok = false;

    qint64 realSize = pimItem.size();

    const QList<QByteArray> list = m_streamParser->readParenthesizedList();
    Q_FOREACH (const QByteArray &item, list) {
        if (partName.isEmpty() && partSize == -1) {
            partName = item;
            continue;
        }
        if (item.startsWith(':')) {
            int pos = 1;
            ImapParser::parseNumber(item, partSize, &ok, pos);
            if (!ok) {
                partSize = 0;
            }

            int version = 0;
            QByteArray plainPartName;
            ImapParser::splitVersionedKey(partName, plainPartName, version);

            partSpecs.append(qMakePair(plainPartName, qMakePair(partSize, version)));
            partName = "";
            partSizes += partSize;
            partSize = -1;
        }
    }

    realSize = qMax(partSizes, realSize);

    const QByteArray allParts = m_streamParser->readString();

    // chop up literal data in parts
    int pos = 0; // traverse through part data now
    QPair<QByteArray, QPair<qint64, int> > partSpec;
    Q_FOREACH (partSpec, partSpecs) {
        // wrap data into a part
        Part part;
        part.setPimItemId(pimItem.id());
        part.setPartType(PartTypeHelper::fromFqName(partSpec.first));
        part.setData(allParts.mid(pos, partSpec.second.first));
        if (partSpec.second.second != 0) {
            part.setVersion(partSpec.second.second);
        }
        part.setDatasize(partSpec.second.first);

        if (!PartHelper::insert(&part)) {
            return failureResponse("Unable to append item part");
        }

        pos += partSpec.second.first;
    }

    if (realSize != pimItem.size()) {
        pimItem.setSize(realSize);
        pimItem.update();
    }

    return true;
}

bool AkAppend::insertItem(Protocol::CreateItemCommand &cmd, PimItem &item)
{
    if (!item.insert()) {
        return failureResponse("Failed to append item");
    }

    Collection parentCol = item.collection();

    // set message flags
    // This will hit an entry in cache inserted there in buildPimItem()
    const Flag::List flagList = HandlerHelper::resolveFlags(cmd.flags());
    bool flagsChanged = false;
    if (!DataStore::self()->appendItemsFlags(PimItem::List() << item, flagList, &flagsChanged, false, parentCol, true)) {
        return failureResponse("Unable to append item flags.");
    }

    const Tag::List tagList = HandlerHelper::resolveTags(cmd.tags(), connection()->context());
    bool tagsChanged = false;
    if (!DataStore::self()->appendItemsTags(PimItem::List() << item, tagList, &tagsChanged, false, parentCol, true)) {
        return failureResponse("Unable to append item tags.");
    }

    // Handle individual parts
    qint64 partSizes = 0;
    // TODO
    if (connection()->capabilities().akAppendStreaming()) {
        QByteArray partName /* unused */;
        qint64 partSize;
        m_streamParser->beginList();
        PartStreamer streamer(connection(), m_streamParser, item, this);
        connect(&streamer, SIGNAL(responseAvailable(Akonadi::Server::Response)),
                this, SIGNAL(responseAvailable(Akonadi::Server::Response)));
        while (!m_streamParser->atListEnd()) {
            QByteArray command = m_streamParser->readString();
            if (command.isEmpty()) {
                throw HandlerException("Syntax error");
            }

            if (!streamer.stream(command, false, partName, partSize)) {
                throw HandlerException(streamer.error());
            }

            partSizes += partSize;
        }

        // TODO: Try to avoid this addition query
        if (partSizes > item.size()) {
            item.setSize(partSizes);
            item.update();
        }
    } else {
        if (!readParts(item)) {
            return false;
        }
    }

    // Preprocessing
    if (PreprocessorManager::instance()->isActive()) {
        Part hiddenAttribute;
        hiddenAttribute.setPimItemId(item.id());
        hiddenAttribute.setPartType(PartTypeHelper::fromFqName(QString::fromLatin1(AKONADI_ATTRIBUTE_HIDDEN)));
        hiddenAttribute.setData(QByteArray());
        hiddenAttribute.setDatasize(0);
        // TODO: Handle errors? Technically, this is not a critical issue as no data are lost
        PartHelper::insert(&hiddenAttribute);
    }

    return true;
}

bool AkAppend::notify(const PimItem &item, const Collection &collection)
{
    DataStore::self()->notificationCollector()->itemAdded(item, collection);

    if (PreprocessorManager::instance()->isActive()) {
        // enqueue the item for preprocessing
        PreprocessorManager::instance()->beginHandleItem(item, DataStore::self());
    }
    return true;
}

bool AkAppend::sendResponse(const QByteArray &responseStr, const PimItem &item)
{
    // Date time is always stored in UTC time zone by the server.
    const QString datetime = QLocale::c().toString(item.datetime(), QLatin1String("dd-MMM-yyyy hh:mm:ss +0000"));

    Response response;
    response.setTag(tag());
    response.setUserDefined();
    response.setString("[UIDNEXT " + QByteArray::number(item.id()) + " DATETIME " + ImapParser::quote(datetime.toUtf8()) + ']');
    Q_EMIT responseAvailable(response);

    response.setSuccess();
    response.setString(responseStr);
    Q_EMIT responseAvailable(response);
    return true;
}

bool AkAppend::parseStream()
{
    Protocol::CreateItemCommand command;
    command << mStream;

    // FIXME: The streaming/reading of all item parts can hold the transaction for
    // unnecessary long time -> should we wrap the PimItem into one transaction
    // and try to insert Parts independently? In case we fail to insert a part,
    // it's not a problem as it can be re-fetched at any time, except for attributes.
    DataStore *db = DataStore::self();
    Transaction transaction(db);

    PimItem item;
    if (!buildPimItem(command, item)) {
        return false;
    }

    if (!insertItem(command, item)) {
        return false;
    }

    // All SQL is done, let's commit!
    if (!transaction.commit()) {
        return failureResponse("Failed to commit transaction");
    }

    notify(item, item.collection());
    return sendResponse("Append completed", item);
}
