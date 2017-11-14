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

namespace Akonadi {
namespace Server {

class IndexFuturePrivate;
class IndexerTask;
class PersistentQueue;
class IndexFuture
{
public:
    ~IndexFuture();
    IndexFuture(const IndexFuture &other);
    IndexFuture &operator=(const IndexFuture &other);
    bool operator==(const IndexFuture &other) const;

    bool isFinished() const;
    bool waitForFinished();

    bool hasError() const;

    qint64 taskId() const;

protected:
    IndexFuture(qint64 taskId);

    void setFinished(bool success);

private:
    QExplicitlySharedDataPointer<IndexFuturePrivate> d;

    friend class Indexer;
    friend class IndexerTask;
    friend class PersistentQueue;
};

}
}

#endif
