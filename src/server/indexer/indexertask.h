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

#ifndef AKONADI_SERVER_INDEXERTASK_H_
#define AKONADI_SERVER_INDEXERTASK_H_

#include <QDataStream>
#include <QDebug>
#include <QDBusArgument>

#include "indexfuture.h"

namespace Akonadi {
namespace Server {

class IndexerTask
{
public:
    enum TaskType {
        Invalid,
        Index,
        Copy,
        Move,
        RemoveItem,
        RemoveCollection
    };

    explicit IndexerTask();

    static IndexerTask createIndexTask(qint64 taskId, qint64 id, const QString &mimeType, const QByteArray &data);
    static IndexerTask createCopyTask(qint64 taskId, qint64 id, const QString &mimeType, qint64 srcCollection, qint64 dstId, qint64 dstCollectionId);
    static IndexerTask createMoveTask(qint64 taskId, qint64 id, const QString &mimeType, qint64 srcCollection, qint64 dstCollectionId);
    static IndexerTask createRemoveItemTask(qint64 taskId, qint64 id, const QString &mimeType);
    static IndexerTask createRemoveCollectionTask(qint64 taskId, qint64 id, const QStringList &mimeTypes);

    bool operator==(const IndexerTask &other) const;

    TaskType taskType = Invalid;
    IndexFuture future;
    qint64 entityId = -1;
    qint64 dstId = -1;
    qint64 sourceCollectionId = -1;
    qint64 destinationCollectionId = -1;
    QStringList mimeTypes;
    QByteArray data;
};

} // namespace Server
} // namespace Akonadi

Q_DECLARE_METATYPE(Akonadi::Server::IndexerTask)

QDebug operator<<(QDebug dbg, const Akonadi::Server::IndexerTask &task);
QDBusArgument &operator<<(QDBusArgument &arg, const Akonadi::Server::IndexerTask &task);

QDataStream &operator<<(QDataStream &stream, const Akonadi::Server::IndexerTask &task);
QDataStream &operator>>(QDataStream &stream, Akonadi::Server::IndexerTask &task);

#endif
