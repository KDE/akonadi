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

#include "persistentqueue.h"
#include "akonadiserver_debug.h"

#include <QDataStream>

using namespace Akonadi::Server;

namespace {

static const ushort CurrentJournalVersion = 1;
static const quint32 OffsetOffset = sizeof(decltype(CurrentJournalVersion));
static const quint32 DataOffset = OffsetOffset + sizeof(quint32); // sizeof(mOffset)

}

PersistentQueue::PersistentQueue()
{
    mStoreFile.close();
}

bool PersistentQueue::setPersistenceFile(const QString &persistenceFile)
{
    mStoreFile.setFileName(persistenceFile);
    if (!openStore()) {
        return false;
    }


    QDataStream stream(&mStoreFile);
    if (mStoreFile.atEnd()) {
        stream << CurrentJournalVersion
               << DataOffset;
        mStoreFile.seek(0);
    }

    ushort version = 0;
    stream >> version;
    stream >> mOffset;

    mStoreFile.seek(mOffset);

    // Load all tasks in the file
    while (!mStoreFile.atEnd()) {
        IndexTask task;
        quint32 size;
        qint64 taskId;
        stream >> size;
        stream >> task.entityId;
        stream >> taskId;
        stream >> size;
        task.future = taskId;
        QByteArray ba;
        ba.resize(size);
        mStoreFile.read(ba.data(), size);
        task.mimeType = QString::fromLatin1(ba);
        stream >> size;
        task.data.resize(size);
        mStoreFile.read(task.data.data(), size);

        mQueue.enqueue(task);
    }

    return true;
}

QString PersistentQueue::persistenceFile() const
{
    return mStoreFile.fileName();
}

IndexTask PersistentQueue::dequeue()
{
    QDataStream stream(&mStoreFile);
    mStoreFile.seek(mOffset);

    quint32 headSize;
    stream >> headSize;
    qDebug() << "dequeue:" << mOffset << (mOffset + sizeof(decltype(headSize)) + headSize) << mStoreFile.size();
    mOffset += sizeof(decltype(headSize)) + headSize;
    if ((mOffset - DataOffset) > (mStoreFile.size() - DataOffset) / 2) {
        const auto &task = mQueue.dequeue();
        rewrite();
        return task;
    } else {
        mStoreFile.seek(OffsetOffset);
        stream << mOffset;
        return mQueue.dequeue();
    }

}

void PersistentQueue::enqueue(const IndexTask &task)
{
    // seek past the end
    mStoreFile.seek(mStoreFile.size());
    writeTask(&mStoreFile, task);
    mStoreFile.flush();
    mQueue.enqueue(task);
}

const IndexTask &PersistentQueue::peekHead() const
{
    return mQueue.first();
}

const IndexTask &PersistentQueue::peekTail() const
{
    return mQueue.last();
}

bool PersistentQueue::isEmpty() const
{
    return mQueue.isEmpty();
}

int PersistentQueue::size() const
{
    return mQueue.size();
}

bool PersistentQueue::openStore()
{
    if (!mStoreFile.open(QIODevice::ReadWrite)) {
        qCCritical(AKONADISERVER_LOG) << "Failed to open rewritten indexer journal:" << mStoreFile.errorString();
        return false;
    }

    return true;
}

bool PersistentQueue::rewrite()
{
    const auto baseName = mStoreFile.fileName();

    QFile f(baseName + QLatin1String(".new"));
    if (!f.open(QIODevice::WriteOnly)) {
        qCCritical(AKONADISERVER_LOG) << "Failed to open" << f.fileName() << ":" << f.errorString();
        return false;
    }

    QDataStream stream(&f);
    stream << CurrentJournalVersion; // version
    stream << DataOffset; // offset
    for (const auto &task : mQueue) {
        writeTask(&f, task);
    }
    f.close();

    mStoreFile.close();
    if (!mStoreFile.rename(baseName + QLatin1Literal("~"))) {
        qCCritical(AKONADISERVER_LOG) << "Failed to create backup file for indexer journal:" << mStoreFile.errorString();
        return false;
    }

    if (!f.rename(baseName)) {
        mStoreFile.rename(baseName);
        openStore();
        qCCritical(AKONADISERVER_LOG) << "Failed to move rewritten indexer journal:" << f.errorString();
        return false;
    }
    mStoreFile.remove();
    mStoreFile.setFileName(baseName);
    return openStore();
}

void PersistentQueue::writeTask(QIODevice *device, const IndexTask &task)
{
    QDataStream stream(device);

    const quint32 size = sizeof(qint64) +   // entityId
                         sizeof(qint64) +   // taskId
                         sizeof(quint32) + task.mimeType.size() + // mimeType
                         sizeof(quint32) + task.data.size();      // data
    stream << size
           << task.entityId
           << task.future.taskId()
           << task.mimeType.size();
    device->write(task.mimeType.toLatin1().constData(), task.mimeType.size());
    stream << task.data.size();
    device->write(task.data.constData(), task.data.size());
}
