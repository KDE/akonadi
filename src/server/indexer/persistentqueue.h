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

#ifndef AKONADI_SERVER_PERSISTENTQUEUE_H_
#define AKONADI_SERVER_PERSISTENTQUEUE_H_

#include <QQueue>
#include <QFile>

#include "indexer/indexertask.h"

namespace Akonadi {
namespace Server {

class PersistentQueue
{
public:
    PersistentQueue();
    virtual ~PersistentQueue() = default;

    bool setPersistenceFile(const QString &persistenceFile);
    QString persistenceFile() const;

    int size() const;
    bool isEmpty() const;
    IndexerTask dequeue();
    const IndexerTask &peekHead() const;
    const IndexerTask &peekTail() const;
    void enqueue(const IndexerTask &obj);

protected:
    virtual bool rewrite();
    bool openStore();
    void writeTask(QIODevice *device, const IndexerTask &task);

    QQueue<IndexerTask> mQueue;
    QFile mStoreFile;
    quint32 mOffset = 0;
};

}
}

#endif
