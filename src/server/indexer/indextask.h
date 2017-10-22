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

#ifndef AKONADI_SERVER_INDEXTASK_H_
#define AKONADI_SERVER_INDEXTASK_H_

#include <QDataStream>
#include <QDebug>
#include <QDBusArgument>

#include "indexfuture.h"

namespace Akonadi {
namespace Server {

class IndexTask
{
public:
    IndexTask()
        : future(-1)
    {}

    IndexTask(qint64 entityId, qint64 taskId, const QString &mimeType, const QByteArray &data)
        : future(taskId)
        , entityId(entityId)
        , mimeType(mimeType)
        , data(data)
    {}

    bool operator==(const IndexTask &other) const
    {
        return future == other.future
                && entityId == other.entityId
                && mimeType == other.mimeType
                && data == other.data;
    }


    IndexFuture future;
    qint64 entityId = -1;
    QString mimeType;
    QByteArray data;
};

} // namespace Server
} // namespace Akonadi

Q_DECLARE_METATYPE(Akonadi::Server::IndexTask)

inline QDebug operator<<(QDebug dbg, const Akonadi::Server::IndexTask &task)
{
    auto d = dbg.nospace();
    d << "IndexTask(entityId: " << task.entityId << ", taskId:" << task.future.taskId()
      << ", mimetype:" << task.mimeType << ")";
    return dbg;
}

inline QDBusArgument &operator<<(QDBusArgument &arg, const Akonadi::Server::IndexTask &task)
{
    arg.beginStructure();
    arg << task.future.taskId()
        << task.entityId
        << task.mimeType;
    arg.endStructure();
    return arg;
}

#endif
