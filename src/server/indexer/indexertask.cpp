/*
    Copyright (c) 2017 Daniel Vrátil <dvratil@kde.org>

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

#include "indexertask.h"

#include "private/datastream_p_p.h"
#include <QBuffer>

using namespace Akonadi;
using namespace Akonadi::Server;

IndexerTask::IndexerTask()
        : taskType(Invalid)
        , future(-1)
    {}

IndexerTask::IndexerTask(qint64 taskId, qint64 itemId, const QString &mimeType, const QByteArray &data)
    : taskType(Index)
    , future(taskId)
    , entityId(itemId)
    , mimeTypes({ mimeType })
    , data(data)
{
}

IndexerTask::IndexerTask(qint64 taskId, qint64 itemId, const QString &mimeType)
    : taskType(RemoveItem)
    , future(taskId)
    , entityId(itemId)
    , mimeTypes({ mimeType })
{
}

IndexerTask::IndexerTask(qint64 taskId, qint64 collectionId, const QStringList &mimeTypes)
    : taskType(RemoveCollection)
    , future(taskId)
    , entityId(collectionId)
    , mimeTypes(mimeTypes)
{
}

IndexerTask::IndexerTask(qint64 taskId, qint64 itemId, const QString &mimeType, qint64 srcCollectionId, qint64 dstCollectionId)
    : taskType(MoveItem)
    , future(taskId)
    , entityId(itemId)
    , collectionId(srcCollectionId)
    , destinationCollectionId(dstCollectionId)
    , mimeTypes({ mimeType })
{
}

bool IndexerTask::operator==(const IndexerTask &other) const
{
    return future == other.future
            && taskType == other.taskType
            && entityId == other.entityId
            && collectionId == other.collectionId
            && destinationCollectionId == other.destinationCollectionId
            && mimeTypes == other.mimeTypes
            && data == other.data;
}

QDebug operator<<(QDebug dbg, const IndexerTask &task)
{
    auto d = dbg.nospace();
    switch (task.taskType) {
    case IndexerTask::Invalid:
        d << "IndexTask(invalid)";
        break;
    case IndexerTask::Index:
        d << "IndexTask(ID:" << task.future.taskId() << ", entityId:" << task.entityId << ")";
        break;
    case IndexerTask::MoveItem:
        d << "MoveItemTask(ID:" << task.future.taskId() << ", entityId:" << task.entityId << ", srcCol:" << task.collectionId << ", destCol:" << task.destinationCollectionId << ")";
        break;
    case IndexerTask::RemoveItem:
        d << "RemoveItemTask(ID:" << task.future.taskId() << ", entityId: " << task.entityId << ")";
        break;
    case IndexerTask::RemoveCollection:
        d << "RemoveCollectionTask(ID:" << task.future.taskId() << ", collectionId:" << task.collectionId << ")";
        break;
    }

    return dbg;
}

QDBusArgument &operator<<(QDBusArgument &arg, const IndexerTask &task)
{
    arg.beginStructure();
    arg << task.future.taskId()
        << task.entityId
        << task.mimeTypes;
    arg.endStructure();
    return arg;
}

QDataStream &operator<<(QDataStream &stream, const IndexerTask &task)
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    Protocol::DataStream ds(&buffer);
    ds << task.entityId
         << task.future.taskId()
         << task.taskType
         << task.mimeTypes
         << task.data
         << task.collectionId
         << task.destinationCollectionId;
    stream << buffer.buffer();
    return stream;
}

QDataStream &operator>>(QDataStream &stream, IndexerTask &task)
{
    QBuffer buffer;
    buffer.open(QIODevice::ReadOnly);
    stream >> buffer.buffer();

    Protocol::DataStream ds(&buffer);
    qint64 taskId;
    ds >> task.entityId
       >> taskId
       >> task.taskType
       >> task.mimeTypes
       >> task.data
       >> task.collectionId
       >> task.destinationCollectionId;
    task.future = IndexFuture(taskId);
    return stream;
}
