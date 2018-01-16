/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

#include "sessionthread_p.h"
#include "session_p.h"
#include "akonadicore_debug.h"

#include <QThread>

Q_DECLARE_METATYPE(Akonadi::Connection::ConnectionType)
Q_DECLARE_METATYPE(Akonadi::Connection *)
Q_DECLARE_METATYPE(Akonadi::CommandBuffer *)

using namespace Akonadi;

SessionThread::SessionThread(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<Connection::ConnectionType>();
    qRegisterMetaType<Connection *>();
    qRegisterMetaType<CommandBuffer *>();

    QThread *thread = new QThread();
    moveToThread(thread);
    thread->start();
}

SessionThread::~SessionThread()
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
    QMetaObject::invokeMethod(this, &SessionThread::doThreadQuit);
#else
    QMetaObject::invokeMethod(this, "doThreadQuit");
#endif
    if (!thread()->wait(10 * 1000)) {
        thread()->terminate();
        // Make sure to wait until it's done, otherwise it can crash when the pthread callback is called
        thread()->wait();
    }
    delete thread();
}

Connection *SessionThread::createConnection(Connection::ConnectionType connectionType,
                                            const QByteArray &sessionId,
                                            CommandBuffer *commandBuffer)
{
    Connection *conn = nullptr;
    const bool invoke = QMetaObject::invokeMethod(this, "doCreateConnection",
                        Qt::BlockingQueuedConnection,
                        Q_RETURN_ARG(Akonadi::Connection*, conn),
                        Q_ARG(Akonadi::Connection::ConnectionType, connectionType),
                        Q_ARG(QByteArray, sessionId),
                        Q_ARG(Akonadi::CommandBuffer*, commandBuffer));
    Q_ASSERT(invoke); Q_UNUSED(invoke);
    return conn;
}

Connection *SessionThread::doCreateConnection(Connection::ConnectionType connType,
                                              const QByteArray &sessionId,
                                              CommandBuffer *commandBuffer)
{
    Q_ASSERT(thread() == QThread::currentThread());

    Connection *conn = new Connection(connType, sessionId, commandBuffer);
    conn->moveToThread(thread());
    connect(conn, &QObject::destroyed,
    this, [this](QObject * obj) {
        mConnections.removeOne(static_cast<Connection *>(obj));
    });
    return conn;
}

void SessionThread::doThreadQuit()
{
    for (Connection *conn : qAsConst(mConnections)) {
        conn->closeConnection();
        delete conn;
    }

    thread()->quit();
}
