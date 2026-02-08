/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QMetaType>
#include <QSharedDataPointer>

namespace Akonadi
{
class CollectionStatisticsPrivate;

/*!
 * \brief Provides statistics information of a Collection.
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
 * \code
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
 * \endcode
 *
 * This class is implicitly shared.
 *
 * \class Akonadi::CollectionStatistics
 * \inheaderfile Akonadi/CollectionStatistics
 * \inmodule AkonadiCore
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADICORE_EXPORT CollectionStatistics
{
    Q_GADGET
    Q_PROPERTY(int count READ count)
    Q_PROPERTY(int size READ size)

public:
    /*!
     * Creates a new collection statistics object.
     */
    CollectionStatistics();

    /*!
     * Creates a collection statistics object from an \a other one.
     */
    CollectionStatistics(const CollectionStatistics &other);

    /*!
     * Destroys the collection statistics object.
     */
    ~CollectionStatistics();

    /*!
     * Returns the number of items in this collection or \\ -1 if
     * this information is not available.
     *
     * \sa setCount()
     * \sa unreadCount()
     */
    [[nodiscard]] qint64 count() const;

    /*!
     * Sets the number of items in this collection.
     *
     * \a count The number of items.
     * \sa count()
     */
    void setCount(qint64 count);

    /*!
     * Returns the number of unread items in this collection or \\ -1 if
     * this information is not available.
     *
     * \sa setUnreadCount()
     * \sa count()
     */
    [[nodiscard]] qint64 unreadCount() const;

    /*!
     * Sets the number of unread items in this collection.
     *
     * \a count The number of unread messages.
     * \sa unreadCount()
     */
    void setUnreadCount(qint64 count);

    /*!
     * Returns the total size of the items in this collection or \\ -1 if
     * this information is not available.
     *
     * \sa setSize()
     * \since 4.3
     */
    [[nodiscard]] qint64 size() const;

    /*!
     * Sets the total size of the items in this collection.
     *
     * \a size The total size of the items
     * \sa size()
     * \since 4.3
     */
    void setSize(qint64 size);

    /*!
     * Assigns \a other to this statistics object and returns a reference to this one.
     */
    CollectionStatistics &operator=(const CollectionStatistics &other);

private:
    QSharedDataPointer<CollectionStatisticsPrivate> d;
};

}

/*!
 * Allows to output the collection statistics for debugging purposes.
 */
AKONADICORE_EXPORT QDebug operator<<(QDebug d, const Akonadi::CollectionStatistics &);

Q_DECLARE_METATYPE(Akonadi::CollectionStatistics)
