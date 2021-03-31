/*
    SPDX-FileCopyrightText: 2006-2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class CollectionStatistics;
class CollectionStatisticsJobPrivate;

/**
 * @short Job that fetches collection statistics from the Akonadi storage.
 *
 * This class fetches the CollectionStatistics object for a given collection.
 *
 * Example:
 *
 * @code
 *
 * Akonadi::Collection collection = ...
 *
 * Akonadi::CollectionStatisticsJob *job = new Akonadi::CollectionStatisticsJob( collection );
 * connect( job, SIGNAL(result(KJob*)), SLOT(jobFinished(KJob*)) );
 *
 * ...
 *
 * MyClass::jobFinished( KJob *job )
 * {
 *   if ( job->error() ) {
 *     qDebug() << "Error occurred";
 *     return;
 *   }
 *
 *   CollectionStatisticsJob *statisticsJob = qobject_cast<CollectionStatisticsJob*>( job );
 *
 *   const Akonadi::CollectionStatistics statistics = statisticsJob->statistics();
 *   qDebug() << "Unread items:" << statistics.unreadCount();
 * }
 *
 * @endcode
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionStatisticsJob : public Job
{
    Q_OBJECT

public:
    /**
     * Creates a new collection statistics job.
     *
     * @param collection The collection to fetch the statistics from.
     * @param parent The parent object.
     */
    explicit CollectionStatisticsJob(const Collection &collection, QObject *parent = nullptr);

    /**
     * Destroys the collection statistics job.
     */
    ~CollectionStatisticsJob() override;

    /**
     * Returns the fetched collection statistics.
     */
    Q_REQUIRED_RESULT CollectionStatistics statistics() const;

    /**
     * Returns the corresponding collection, if the job was executed successfully,
     * the collection is already updated.
     */
    Q_REQUIRED_RESULT Collection collection() const;

protected:
    void doStart() override;
    bool doHandleResponse(qint64 tag, const Protocol::CommandPtr &response) override;

private:
    Q_DECLARE_PRIVATE(CollectionStatisticsJob)
};

}

