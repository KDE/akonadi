/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "collectionstatisticsjob.h"

#include "collection.h"
#include "collectionstatistics.h"
#include <akonadi/private/imapparser_p.h>
#include "job_p.h"

#include <qdebug.h>

using namespace Akonadi;

class Akonadi::CollectionStatisticsJobPrivate : public JobPrivate
{
public:
    CollectionStatisticsJobPrivate(CollectionStatisticsJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const Q_DECL_OVERRIDE { /*Q_DECL_OVERRIDE*/
        return QStringLiteral("Collection Id %1").arg(mCollection.id());
    }

    Collection mCollection;
    CollectionStatistics mStatistics;
};

CollectionStatisticsJob::CollectionStatisticsJob(const Collection &collection, QObject *parent)
    : Job(new CollectionStatisticsJobPrivate(this), parent)
{
    Q_D(CollectionStatisticsJob);

    d->mCollection = collection;
}

CollectionStatisticsJob::~CollectionStatisticsJob()
{
}

void CollectionStatisticsJob::doStart()
{
    Q_D(CollectionStatisticsJob);

    d->writeData(d->newTag() + " STATUS " + QByteArray::number(d->mCollection.id()) + " (MESSAGES UNSEEN SIZE)\n");
}

void CollectionStatisticsJob::doHandleResponse(const QByteArray &tag, const QByteArray &data)
{
    Q_D(CollectionStatisticsJob);

    if (tag == "*") {
        QByteArray token;
        int current = ImapParser::parseString(data, token);
        if (token == "STATUS") {
            // folder path
            current = ImapParser::parseString(data, token, current);
            // result list
            QList<QByteArray> list;
            current = ImapParser::parseParenthesizedList(data, list, current);
            for (int i = 0; i < list.count() - 1; i += 2) {
                if (list[i] == "MESSAGES") {
                    d->mStatistics.setCount(list[i + 1].toLongLong());
                } else if (list[i] == "UNSEEN") {
                    d->mStatistics.setUnreadCount(list[i + 1].toLongLong());
                } else if (list[i] == "SIZE") {
                    d->mStatistics.setSize(list[i + 1].toLongLong());
                } else {
                    qDebug() << "Unknown STATUS response: " << list[i];
                }
            }

            d->mCollection.setStatistics(d->mStatistics);
            return;
        }
    }
    qDebug() << "Unhandled response: " << tag << data;
}

Collection CollectionStatisticsJob::collection() const
{
    Q_D(const CollectionStatisticsJob);

    return d->mCollection;
}

CollectionStatistics Akonadi::CollectionStatisticsJob::statistics() const
{
    Q_D(const CollectionStatisticsJob);

    return d->mStatistics;
}
