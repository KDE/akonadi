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
#include <QEventLoop>
#include <QTimer>

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

    QTimer::singleShot(0, this, &SessionThread::waitForSocketData);
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

void SessionThread::addConnection(Connection *connection)
{
    connection->moveToThread(thread());
    const bool invoke = QMetaObject::invokeMethod(this, "doAddConnection",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_ARG(Akonadi::Connection*, connection));
    Q_ASSERT(invoke); Q_UNUSED(invoke);
}

void SessionThread::doAddConnection(Connection *connection)
{
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(!mConnections.contains(connection));

    connect(connection, &QObject::destroyed,
            this, [this](QObject * obj) {
                mConnections.removeOne(static_cast<Connection *>(obj));
                if (mWaitLoop) {
                    mWaitLoop->quit();
                }
            });

    mConnections.push_back(connection);

    if (mWaitLoop) {
        mWaitLoop->quit();
    }
    if (!mWaiting) {
        mWaiting = true;
        QTimer::singleShot(0, this, &SessionThread::waitForSocketData);
    }

    return conn;
}

void SessionThread::destroyConnection(Connection *connection)
{
    const bool invoke = QMetaObject::invokeMethod(this, "doDestroyConnection",
                                                  Qt::BlockingQueuedConnection,
                                                  Q_ARG(Akonadi::Connection*, connection));
    Q_ASSERT(invoke); Q_UNUSED(invoke);
}

void SessionThread::doDestroyConnection(Connection *connection)
{
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(mConnections.contains(connection));

    connection->disconnect(this);
    connection->doCloseConnection();
    mConnections.removeAll(connection);
    delete connection;

    if (mWaitLoop) {
        mWaitLoop->quit();
    }

    if (!mWaiting) {
        mWaiting = true;
        QTimer::singleShot(0, this, &SessionThread::waitForSocketData);
    }
}

void SessionThread::doThreadQuit()
{
    Q_ASSERT(thread() == QThread::currentThread());

    for (Connection *conn : qAsConst(mConnections)) {
        if (mWaitLoop) {
            conn->socket()->disconnect(mWaitLoop);
        }
        conn->disconnect(this);
        conn->doCloseConnection(); // we can call directly because we are in the correct thread
        delete conn;
    }
    if (mWaitLoop) {
        mWaitLoop->quit();
    }

    thread()->quit();
}

void SessionThread::waitForSocketData()
{
    mWaiting = true;
    while (!mConnections.isEmpty()) {
        QEventLoop loop;
        mWaitLoop = &loop;
        for (auto con : qAsConst(mConnections)) {
            auto socket = con->socket();
            if (socket) {
                connect(socket, &QLocalSocket::readyRead, &loop, &QEventLoop::quit);
                connect(socket, &QLocalSocket::disconnected, &loop, &QEventLoop::quit);
            } else {
                connect(con, &Connection::reconnected, &loop, &QEventLoop::quit);
            }
        }
        loop.exec();
        mWaitLoop = nullptr;

        for (auto con : qAsConst(mConnections)) {
            con->handleIncomingData();
        }
    }
    mWaiting = false;
}
