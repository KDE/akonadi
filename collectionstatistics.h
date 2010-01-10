/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_COLLECTIONSTATISTICS_H
#define AKONADI_COLLECTIONSTATISTICS_H

#include "akonadi_export.h"
#include <QtCore/QMetaType>
#include <QtCore/QSharedDataPointer>

namespace Akonadi {

/**
 * @short Provides statistics information of a Collection.
 *
 * This class contains information such as total number of items,
 * number of new and unread items, etc.
 *
 * These information might be expensive to obtain and are thus
 * not included when fetching collection with a CollectionFetchJob.
 * They can be retrieved spearately using CollectionStatisticsJob.
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
 * This class is implicitely shared.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT CollectionStatistics
{
  public:
    /**
     * Creates a new collection statistics object.
     */
    CollectionStatistics();

    /**
     * Creates a collection statistics object from an @p other one.
     */
    CollectionStatistics( const CollectionStatistics &other );

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
    qint64 count() const;

    /**
     * Sets the number of items in this collection.
     *
     * @param count The number of items.
     * @see count()
     */
    void setCount( qint64 count );

    /**
     * Returns the number of unread items in this collection or @c -1 if
     * this information is not available.
     *
     * @see setUnreadCount()
     * @see count()
     */
    qint64 unreadCount() const;

    /**
     * Sets the number of unread items in this collection.
     *
     * @param count The number of unread messages.
     * @see unreadCount()
     */
    void setUnreadCount( qint64 count );

    /**
     * Returns the total size of the items in this collection or @c -1 if
     * this information is not available.
     *
     * @see setSize()
     * @since 4.3
     */
    qint64 size() const;

    /**
     * Sets the total size of the items in this collection.
     *
     * @param size The total size of the items
     * @see size()
     * @since 4.3
     */
    void setSize( qint64 size );

    /**
     * Assigns @p other to this statistics object and returns a reference to this one.
     */
    CollectionStatistics& operator=( const CollectionStatistics &other );

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
AKONADI_EXPORT QDebug operator<<( QDebug d, const Akonadi::CollectionStatistics& );

Q_DECLARE_METATYPE( Akonadi::CollectionStatistics )

#endif
