/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akonadicore_debug.h"
#include "session_p.h"
#include "sessionthread_p.h"

#include <QCoreApplication>
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

    auto thread = new QThread();
    thread->setObjectName(QLatin1StringView("SessionThread"));
    moveToThread(thread);
    thread->start();
}

SessionThread::~SessionThread()
{
    QMetaObject::invokeMethod(this, &SessionThread::doThreadQuit, Qt::QueuedConnection);
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
    const bool invoke = QMetaObject::invokeMethod(this, "doAddConnection", Qt::BlockingQueuedConnection, Q_ARG(Akonadi::Connection *, connection));
    Q_ASSERT(invoke);
    Q_UNUSED(invoke)
}

void SessionThread::doAddConnection(Connection *connection)
{
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(!mConnections.contains(connection));

    connect(connection, &QObject::destroyed, this, [this](QObject *obj) {
        mConnections.removeOne(static_cast<Connection *>(obj));
    });
    mConnections.push_back(connection);
}

void SessionThread::destroyConnection(Connection *connection)
{
    if (QCoreApplication::closingDown()) {
        return;
    }

    const bool invoke = QMetaObject::invokeMethod(this, "doDestroyConnection", Qt::BlockingQueuedConnection, Q_ARG(Akonadi::Connection *, connection));
    Q_ASSERT(invoke);
    Q_UNUSED(invoke)
}

void SessionThread::doDestroyConnection(Connection *connection)
{
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(mConnections.contains(connection));

    connection->disconnect(this);
    connection->doCloseConnection();
    mConnections.removeAll(connection);
    delete connection;
}

void SessionThread::doThreadQuit()
{
    Q_ASSERT(thread() == QThread::currentThread());

    for (Connection *conn : std::as_const(mConnections)) {
        conn->disconnect(this);
        conn->doCloseConnection(); // we can call directly because we are in the correct thread
        delete conn;
    }

    thread()->quit();
}

#include "moc_sessionthread_p.cpp"
