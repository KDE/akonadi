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

#include "libs/imapparser_p.h"
#include "imapstreamparser.h"

#include "append.h"
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
#include "libs/protocol_p.h"

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

bool AkAppend::buildPimItem(PimItem &item, Collection &col,
                            ChangedAttributes &itemFlags,
                            ChangedAttributes &itemTagsRID,
                            ChangedAttributes &itemTagsGID)
{
    // Arguments:  mailbox name
    //        OPTIONAL flag parenthesized list
    //        OPTIONAL date/time string
    //        (partname literal)+
    //
    // Syntax:
    // x-akappend = "X-AKAPPEND" SP mailbox SP size [SP flag-list] [SP date-time] SP (partname SP literal)+
    const QByteArray mailbox = m_streamParser->readString();

    const qint64 size = m_streamParser->readNumber();
    // parse optional flag parenthesized list
    // Syntax:
    // flag-list      = "(" [flag *(SP flag)] ")"
    // flag           = "\ANSWERED" / "\FLAGGED" / "\DELETED" / "\SEEN" /
    //                  "\DRAFT" / flag-keyword / flag-extension
    //                    ; Does not include "\Recent"
    // flag-extension = "\" atom
    // flag-keyword   = atom
    QList<QByteArray> flags;
    if (m_streamParser->hasList()) {
        flags = m_streamParser->readParenthesizedList();
    }

    // parse optional date/time string
    QDateTime dateTime;
    if (m_streamParser->hasDateTime()) {
        dateTime = m_streamParser->readDateTime().toUTC();
        // FIXME Should we return an error if m_dateTime is invalid?
    } else {
        // if date/time is not given then it will be set to the current date/time
        // converted to UTC.
        dateTime = QDateTime::currentDateTime().toUTC();
    }

    Response response;

    col = HandlerHelper::collectionFromIdOrName(mailbox);
    if (!col.isValid()) {
        throw HandlerException(QByteArray("Unknown collection for '") + mailbox + QByteArray("'."));
    }
    if (col.isVirtual()) {
        throw HandlerException("Cannot append item into virtual collection");
    }

    QByteArray mt;
    QString remote_id;
    QString remote_revision;
    QString gid;
    Q_FOREACH (const QByteArray &flag, flags) {
        if (flag.startsWith(AKONADI_FLAG_MIMETYPE)) {
            mt = parseFlag(flag);
        } else if (flag.startsWith(AKONADI_FLAG_REMOTEID)) {
            remote_id = QString::fromUtf8(parseFlag(flag));
        } else if (flag.startsWith(AKONADI_FLAG_REMOTEREVISION)) {
            remote_revision = QString::fromUtf8(parseFlag(flag));
        } else if (flag.startsWith(AKONADI_FLAG_GID)) {
            gid = QString::fromUtf8(parseFlag(flag));
        } else if (flag.startsWith("+" AKONADI_FLAG_TAG)) {
            itemTagsGID.incremental = true;
            itemTagsGID.added.append(parseFlag(flag));
        } else if (flag.startsWith("-" AKONADI_FLAG_TAG)) {
            itemTagsGID.incremental = true;
            itemTagsGID.removed.append(parseFlag(flag));
        } else if (flag.startsWith(AKONADI_FLAG_TAG)) {
            itemTagsGID.incremental = false;
            itemTagsGID.added.append(parseFlag(flag));
        } else if (flag.startsWith("+" AKONADI_FLAG_RTAG)) {
            itemTagsRID.incremental = true;
            itemTagsRID.added.append(parseFlag(flag));
        } else if (flag.startsWith("-" AKONADI_FLAG_RTAG)) {
            itemTagsRID.incremental = true;
            itemTagsRID.removed.append(parseFlag(flag));
        } else if (flag.startsWith(AKONADI_FLAG_RTAG)) {
            itemTagsRID.incremental = false;
            itemTagsRID.added.append(parseFlag(flag));
        } else if (flag.startsWith('+')) {
            itemFlags.incremental = true;
            itemFlags.added.append(flag.mid(1));
        } else if (flag.startsWith('-')) {
            itemFlags.incremental = true;
            itemFlags.removed.append(flag.mid(1));
        } else {
            itemFlags.incremental = false;
            itemFlags.added.append(flag);
        }
    }
    // standard imap does not know this attribute, so that's mail
    if (mt.isEmpty()) {
        mt = "message/rfc822";
    }
    MimeType mimeType = MimeType::retrieveByName(QString::fromLatin1(mt));
    if (!mimeType.isValid()) {
        MimeType m(QString::fromLatin1(mt));
        if (!m.insert()) {
            return failureResponse(QByteArray("Unable to create mimetype '") + mt + QByteArray("'."));
        }
        mimeType = m;
    }

    item.setRev(0);
    item.setSize(size);
    item.setMimeTypeId(mimeType.id());
    item.setCollectionId(col.id());
    if (dateTime.isValid()) {
        item.setDatetime(dateTime);
    }
    if (remote_id.isEmpty()) {
        // from application
        item.setDirty(true);
    } else {
        // from resource
        item.setRemoteId(remote_id);
        item.setDirty(false);
    }
    item.setRemoteRevision(remote_revision);
    item.setGid(gid);
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

bool AkAppend::insertItem(PimItem &item, const Collection &parentCol,
                          const QVector<QByteArray> &itemFlags,
                          const QVector<QByteArray> &itemTagsRID,
                          const QVector<QByteArray> &itemTagsGID)
{
    if (!item.insert()) {
        return failureResponse("Failed to append item");
    }

    // set message flags
    // This will hit an entry in cache inserted there in buildPimItem()
    const Flag::List flagList = HandlerHelper::resolveFlags(itemFlags);
    bool flagsChanged = false;
    if (!DataStore::self()->appendItemsFlags(PimItem::List() << item, flagList, &flagsChanged, false, parentCol, true)) {
        return failureResponse("Unable to append item flags.");
    }

    Tag::List tagList;
    if (!itemTagsGID.isEmpty()) {
        tagList << HandlerHelper::resolveTagsByGID(itemTagsGID);
    }
    if (!itemTagsRID.isEmpty()) {
        tagList << HandlerHelper::resolveTagsByRID(itemTagsRID, connection()->context());
    }
    bool tagsChanged;
    if (!DataStore::self()->appendItemsTags(PimItem::List() << item, tagList, &tagsChanged, false, parentCol, true)) {
        return failureResponse("Unable to append item tags.");
    }

    // Handle individual parts
    qint64 partSizes = 0;
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
    // FIXME: The streaming/reading of all item parts can hold the transaction for
    // unnecessary long time -> should we wrap the PimItem into one transaction
    // and try to insert Parts independently? In case we fail to insert a part,
    // it's not a problem as it can be re-fetched at any time, except for attributes.
    DataStore *db = DataStore::self();
    Transaction transaction(db);

    ChangedAttributes itemFlags, itemTagsRID, itemTagsGID;
    Collection parentCol;
    PimItem item;
    if (!buildPimItem(item, parentCol, itemFlags, itemTagsRID, itemTagsGID)) {
        return false;
    }

    if (itemFlags.incremental) {
        throw HandlerException("Incremental flags changes are not allowed in AK-APPEND");
    }
    if (itemTagsRID.incremental || itemTagsRID.incremental) {
        throw HandlerException("Incremental tags changes are not allowed in AK-APPEND");
    }

    if (!insertItem(item, parentCol, itemFlags.added, itemTagsRID.added, itemTagsGID.added)) {
        return false;
    }

    // All SQL is done, let's commit!
    if (!transaction.commit()) {
        return failureResponse("Failed to commit transaction");
    }

    notify(item, parentCol);
    return sendResponse("Append completed", item);
}
