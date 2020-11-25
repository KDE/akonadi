/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akthread.h"
#include "storage/datastore.h"
#include "akonadiserver_debug.h"


using namespace Akonadi::Server;

AkThread::AkThread(const QString &objectName, StartMode startMode, QThread::Priority priority, QObject *parent)
    : QObject(parent), m_startMode(startMode)
{
    setObjectName(objectName);
    if (startMode != NoThread) {
        auto *thread = new QThread();
        thread->setObjectName(objectName + QStringLiteral("-Thread"));
        moveToThread(thread);
        thread->start(priority);
    }

    if (startMode == AutoStart) {
        startThread();
    }
}

AkThread::AkThread(const QString &objectName, QThread::Priority priority, QObject *parent)
    : AkThread(objectName, AutoStart, priority, parent)
{
}

AkThread::~AkThread() = default;

void AkThread::startThread()
{
    Q_ASSERT(m_startMode != NoThread);
    const bool init = QMetaObject::invokeMethod(this, &AkThread::init, Qt::QueuedConnection);
    Q_ASSERT(init);
    Q_UNUSED(init)
}

void AkThread::quitThread()
{
    if (m_startMode == NoThread) {
        return;
    }
    qCDebug(AKONADISERVER_LOG) << "Shutting down" << objectName() << "...";
    const bool invoke = QMetaObject::invokeMethod(this, &AkThread::quit, Qt::QueuedConnection);

    Q_ASSERT(invoke);
    Q_UNUSED(invoke)
    if (!thread()->wait(10 * 1000)) {
        thread()->terminate();
        thread()->wait();
    }
    delete thread();
}

void AkThread::init()
{
    Q_ASSERT(thread() == QThread::currentThread());
}

void AkThread::quit()
{
    Q_ASSERT(thread() == QThread::currentThread());

    if (DataStore::hasDataStore()) {
        DataStore::self()->close();
    }

    if (m_startMode != NoThread) {
        thread()->quit();
    }
}
