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

#ifndef AKONADI_INDEXERFUTURE_H_
#define AKONADI_INDEXERFUTURE_H_

#include <QExplicitlySharedDataPointer>
#include <QSet>

namespace Akonadi {
namespace Server {

class IndexFuturePrivate;
class IndexerTask;
class PersistentQueue;
class IndexFutureSet;
class IndexFuture
{
public:
    ~IndexFuture();
    IndexFuture(qint64 taskId);
    IndexFuture(const IndexFuture &other);
    IndexFuture &operator=(const IndexFuture &other);
    bool operator==(const IndexFuture &other) const;

    bool isFinished() const;
    bool waitForFinished();

    bool hasError() const;

    qint64 taskId() const;

protected:

    void setFinished(bool success);

private:
    void setFutureSet(IndexFutureSet *set);

    QExplicitlySharedDataPointer<IndexFuturePrivate> d;

    friend class Indexer;
    friend class IndexerTask;
    friend class PersistentQueue;
    friend class IndexFutureSet;
};


class IndexFutureSet
{
public:
    explicit IndexFutureSet();
    explicit IndexFutureSet(int reserveSize);
    void add(const IndexFuture &future);

    void waitForAll();

private:
    QSet<IndexFuture> mFutures;

    friend class IndexFuture;
};


}
}

#endif
