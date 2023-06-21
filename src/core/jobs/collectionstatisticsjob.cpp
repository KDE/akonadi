/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionstatisticsjob.h"

#include "collection.h"
#include "collectionstatistics.h"
#include "job_p.h"
#include "private/protocol_p.h"
#include "protocolhelper_p.h"

using namespace Akonadi;

class Akonadi::CollectionStatisticsJobPrivate : public JobPrivate
{
public:
    explicit CollectionStatisticsJobPrivate(CollectionStatisticsJob *parent)
        : JobPrivate(parent)
    {
    }

    QString jobDebuggingString() const override
    {
        return QStringLiteral("Collection Statistic from collection Id %1").arg(mCollection.id());
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

    try {
        d->sendCommand(Protocol::FetchCollectionStatsCommandPtr::create(ProtocolHelper::entityToScope(d->mCollection)));
    } catch (const std::exception &e) {
        setError(Unknown);
        setErrorText(QString::fromUtf8(e.what()));
        emitResult();
        return;
    }
}

bool CollectionStatisticsJob::doHandleResponse(qint64 tag, const Protocol::CommandPtr &response)
{
    Q_D(CollectionStatisticsJob);

    if (!response->isResponse() || response->type() != Protocol::Command::FetchCollectionStats) {
        return Job::doHandleResponse(tag, response);
    }

    d->mStatistics = ProtocolHelper::parseCollectionStatistics(Protocol::cmdCast<Protocol::FetchCollectionStatsResponse>(response));
    return true;
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

#include "moc_collectionstatisticsjob.cpp"
