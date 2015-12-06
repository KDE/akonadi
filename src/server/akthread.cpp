/*
 * Copyright 2015  Daniel Vrátil <dvratil@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "akthread.h"
#include "storage/datastore.h"

#include <QThread>
#include <QCoreApplication>

using namespace Akonadi::Server;

AkThread::AkThread(QThread::Priority priority, QObject *parent)
    : QObject(parent)
{
    QThread *thread = new QThread();
    moveToThread(thread);
    thread->start(priority);

    const bool init = QMetaObject::invokeMethod(this, "init", Qt::QueuedConnection);
    Q_ASSERT(init); Q_UNUSED(init);
}

AkThread::~AkThread()
{
}

void AkThread::quitThread()
{
    akDebug() << "Shutting down" << objectName() << "...";
    const bool invoke = QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);
    Q_ASSERT(invoke); Q_UNUSED(invoke);
    if (!thread()->wait(10 * 1000)) {
        thread()->terminate();
        thread()->wait();
    }
    delete thread();
}

void AkThread::init()
{
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(thread() != qApp->thread());
}

void AkThread::quit()
{
    Q_ASSERT(thread() == QThread::currentThread());
    Q_ASSERT(thread() != qApp->thread());

    if (DataStore::hasDataStore()) {
        DataStore::self()->close();
    }

    thread()->quit();
}


