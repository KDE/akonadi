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

#ifndef AKONADI_COLLECTIONSTATISTICSJOB_H
#define AKONADI_COLLECTIONSTATISTICSJOB_H

#include "akonadi_export.h"

#include <akonadi/job.h>

namespace Akonadi {

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
 * connect( job, SIGNAL( result( KJob* ) ), SLOT( jobFinished( KJob* ) ) );
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
class AKONADI_EXPORT CollectionStatisticsJob : public Job
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection statistics job.
     *
     * @param collection The collection to fetch the statistics from.
     * @param parent The parent object.
     */
    explicit CollectionStatisticsJob( const Collection &collection, QObject *parent = 0 );

    /**
     * Destroys the collection statistics job.
     */
    virtual ~CollectionStatisticsJob();

    /**
     * Returns the fetched collection statistics.
     */
    CollectionStatistics statistics() const;

    /**
     * Returns the corresponding collection, if the job was executed successfully,
     * the collection is already updated.
     */
    Collection collection() const;

  protected:
    virtual void doStart();
    virtual void doHandleResponse( const QByteArray &tag, const QByteArray &data );

  private:
    Q_DECLARE_PRIVATE( CollectionStatisticsJob )
};

}

#endif
