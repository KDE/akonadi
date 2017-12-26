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

#ifndef AKONADI_INDEXER_H_
#define AKONADI_INDEXER_H_

#include "akthread.h"
#include "indexertask.h"

#include <QList>
#include <QExplicitlySharedDataPointer>

namespace Akonadi {
namespace Server {

class IndexFuture;
class IndexerPrivate;
class Indexer : public AkThread
{
    Q_OBJECT

public:
    static Indexer *instance();

    explicit Indexer();
    ~Indexer();

    IndexFuture index(qint64 id, const QString &mimeType, const QByteArray &indexData);
    IndexFuture copy(qint64 id, const QString &mimeType, qint64 sourceCollection,
                     qint64 newId, qint64 destinationCollection);
    IndexFuture move(qint64 id, const QString &mimeType, qint64 sourceCollection,
                     qint64 destinationCollection);
    IndexFuture removeItem(qint64 id, const QString &mimeType);
    IndexFuture removeCollection(qint64 colId, const QStringList &mimeTypes);

    void init() override;
    void quit() override;

    void enableDebugging(bool enable);
    QList<IndexerTask> queue();

Q_SIGNALS:
    void enqueued(qint64 taskId, qint64 itemId, const QString &mimeType);
    void processed(qint64 taskId, bool success);

private:
    void indexerLoop();

    IndexerPrivate * const d;

    static Indexer *sInstance;
};

} // namespace Server
} // namespace Akonadi

#endif
