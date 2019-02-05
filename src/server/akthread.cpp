/*
    Copyright (c) 2015 Daniel Vrátil <dvratil@kde.org>

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

#include "akthread.h"
#include "storage/datastore.h"
#include "akonadiserver_debug.h"

#include <QThread>
#include <QCoreApplication>

using namespace Akonadi::Server;

AkThread::AkThread(const QString &objectName, StartMode startMode, QThread::Priority priority, QObject *parent)
    : QObject(parent), m_startMode(startMode)
{
    setObjectName(objectName);
    if (startMode != NoThread) {
        QThread *thread = new QThread();
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

AkThread::~AkThread()
{
}

void AkThread::startThread()
{
    Q_ASSERT(m_startMode != NoThread);
    const bool init = QMetaObject::invokeMethod(this, &AkThread::init, Qt::QueuedConnection);
    Q_ASSERT(init);
    Q_UNUSED(init);
}

void AkThread::quitThread()
{
    if (m_startMode == NoThread)
        return;
    qCDebug(AKONADISERVER_LOG) << "Shutting down" << objectName() << "...";
    const bool invoke = QMetaObject::invokeMethod(this, &AkThread::quit, Qt::QueuedConnection);

    Q_ASSERT(invoke);
    Q_UNUSED(invoke);
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
    Q_ASSERT(m_startMode != NoThread);
    Q_ASSERT(thread() == QThread::currentThread());

    if (DataStore::hasDataStore()) {
        DataStore::self()->close();
    }

    thread()->quit();
}
