/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONSTATISTICS_H
#define AKONADI_COLLECTIONSTATISTICS_H

#include "akonadicore_export.h"

#include <QMetaType>
#include <QSharedDataPointer>

namespace Akonadi
{

/**
 * @short Provides statistics information of a Collection.
 *
 * This class contains information such as total number of items,
 * number of new and unread items, etc.
 *
 * This information might be expensive to obtain and is thus
 * not included when fetching collections with a CollectionFetchJob.
 * It can be retrieved separately using CollectionStatisticsJob.
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
 * This class is implicitly shared.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionStatistics
{
public:
    /**
     * Creates a new collection statistics object.
     */
    CollectionStatistics();

    /**
     * Creates a collection statistics object from an @p other one.
     */
    CollectionStatistics(const CollectionStatistics &other);

    /**
     * Destroys the collection statistics object.
     */
    ~CollectionStatistics();

    /**
     * Returns the number of items in this collection or @c -1 if
     * this information is not available.
     *
     * @see setCount()
     * @see unreadCount()
     */
    Q_REQUIRED_RESULT qint64 count() const;

    /**
     * Sets the number of items in this collection.
     *
     * @param count The number of items.
     * @see count()
     */
    void setCount(qint64 count);

    /**
     * Returns the number of unread items in this collection or @c -1 if
     * this information is not available.
     *
     * @see setUnreadCount()
     * @see count()
     */
    Q_REQUIRED_RESULT qint64 unreadCount() const;

    /**
     * Sets the number of unread items in this collection.
     *
     * @param count The number of unread messages.
     * @see unreadCount()
     */
    void setUnreadCount(qint64 count);

    /**
     * Returns the total size of the items in this collection or @c -1 if
     * this information is not available.
     *
     * @see setSize()
     * @since 4.3
     */
    Q_REQUIRED_RESULT qint64 size() const;

    /**
     * Sets the total size of the items in this collection.
     *
     * @param size The total size of the items
     * @see size()
     * @since 4.3
     */
    void setSize(qint64 size);

    /**
     * Assigns @p other to this statistics object and returns a reference to this one.
     */
    CollectionStatistics &operator=(const CollectionStatistics &other);

private:
    //@cond PRIVATE
    class Private;
    QSharedDataPointer<Private> d;
    //@endcond
};

}

/**
 * Allows to output the collection statistics for debugging purposes.
 */
AKONADICORE_EXPORT QDebug operator<<(QDebug d, const Akonadi::CollectionStatistics &);

Q_DECLARE_METATYPE(Akonadi::CollectionStatistics)

#endif
