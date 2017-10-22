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

#include "indexfuture.h"

#include <QMutex>
#include <QWaitCondition>

namespace Akonadi {
namespace Server {

class IndexFuturePrivate : public QSharedData
{
public:
    IndexFuturePrivate(qint64 taskId)
        : taskId(taskId)
    {}

    const qint64 taskId;
    QMutex lock;
    QWaitCondition cond;
    bool finished = false;
    bool hasError = false;
};

}
}

using namespace Akonadi::Server;


IndexFuture::IndexFuture(qint64 taskId)
    : d(new IndexFuturePrivate(taskId))
{
}

IndexFuture::IndexFuture(const IndexFuture &other)
    : d(other.d)
{
}

IndexFuture::~IndexFuture()
{
}

IndexFuture &IndexFuture::operator=(const IndexFuture &other)
{
    d = other.d;
    return *this;
}

bool IndexFuture::operator==(const IndexFuture &other) const
{
    return d->taskId == other.d->taskId;
}

bool IndexFuture::isFinished() const
{
    QMutexLocker lock(&d->lock);
    return d->finished;
}

void IndexFuture::setFinished(bool success)
{
    QMutexLocker lock(&d->lock);
    d->finished = true;
    d->hasError = !success;
    lock.unlock();
    d->cond.wakeAll();
}

qint64 IndexFuture::taskId() const
{
    // TaskId is const and thus thread-safe
    return d->taskId;
}

bool IndexFuture::hasError() const
{
    QMutexLocker lock(&d->lock);
    return d->hasError;
}


bool IndexFuture::waitForFinished()
{
    QMutexLocker lock(&d->lock);
    if (d->finished) {
        return true;
    }
    return d->cond.wait(&d->lock);
}
