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

IndexerTask IndexerTask::createIndexTask(qint64 taskId, qint64 id, const QString &mimeType, const QByteArray &data)
{
    IndexerTask task;
    task.taskType = Index;
    task.entityId = id;
    task.future = IndexFuture(taskId);
    task.mimeTypes << mimeType;
    task.data = data;
    return task;
}

IndexerTask IndexerTask::createCopyTask(qint64 taskId, qint64 id, const QString &mimeType, qint64 sourceCollectionId,
                                        qint64 dstId, qint64 dstCollectionId)
{
    IndexerTask task;
    task.taskType = Copy;
    task.entityId = id;
    task.future = IndexFuture(taskId);
    task.mimeTypes << mimeType;
    task.dstId = dstId;
    task.sourceCollectionId = sourceCollectionId;
    task.destinationCollectionId = dstCollectionId;
    return task;
}

IndexerTask IndexerTask::createMoveTask(qint64 taskId, qint64 id, const QString &mimeType, qint64 sourceCollectionId, qint64 dstCollectionId)
{
    IndexerTask task;
    task.taskType = Move;
    task.entityId = id;
    task.future = IndexFuture(taskId);
    task.mimeTypes << mimeType;
    task.sourceCollectionId = sourceCollectionId;
    task.destinationCollectionId = dstCollectionId;
    return task;
}

IndexerTask IndexerTask::createRemoveItemTask(qint64 taskId, qint64 id, const QString &mimeType)
{
    IndexerTask task;
    task.taskType = RemoveItem;
    task.entityId = id;
    task.mimeTypes << mimeType;
    return task;
}


IndexerTask IndexerTask::createRemoveCollectionTask(qint64 taskId, qint64 id, const QStringList &mimeTypes)
{
    IndexerTask task;
    task.taskType = RemoveCollection;
    task.entityId = id;
    task.mimeTypes = mimeTypes;
    return task;
}

bool IndexerTask::operator==(const IndexerTask &other) const
{
    return future == other.future
            && taskType == other.taskType
            && entityId == other.entityId
            && dstId == other.dstId
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
    case IndexerTask::Copy:
        d << "CopyTask(ID:" << task.future.taskId() << ", entityId:" << task.entityId
          << ", sourceCol:" << task.sourceCollectionId << ", destId:" << task.dstId
          << ", destCol:" << task.destinationCollectionId << ")";
        break;
    case IndexerTask::Move:
        d << "MoveTask(ID:" << task.future.taskId() << ", entityId:" << task.entityId
          << ", sourceCol:" << task.sourceCollectionId << ", destCol:" << task.destinationCollectionId << ")";
        break;
    case IndexerTask::RemoveItem:
        d << "RemoveItemTask(ID:" << task.future.taskId() << ", entityId: " << task.entityId << ")";
        break;
    case IndexerTask::RemoveCollection:
        d << "RemoveCollectionTask(ID:" << task.future.taskId() << ", entityId:" << task.entityId << ")";
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
         << task.dstId
         << task.sourceCollectionId
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
       >> task.dstId
       >> task.sourceCollectionId
       >> task.destinationCollectionId;
    task.future = IndexFuture(taskId);
    return stream;
}
