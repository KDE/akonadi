/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_CACHEPOLICY_H
#define AKONADI_CACHEPOLICY_H

#include "akonadi_export.h"

#include <QtCore/QSharedDataPointer>
#include <QtCore/QStringList>

namespace Akonadi {

/**
 * @short Represents the caching policy for a collection.
 *
 * There is one cache policy per collection, it can either define to
 * inherit all properties of the policy of the parent collection (the default)
 * or specify the following values:
 *
 * - The item parts that should be permanently kept locally and are downloaded
 *   during a collection sync (eg. full mail vs. just the headers).
 * - A time up to which non-permantly cached item parts have to be kept at
 *   least (0 - infinity).
 * - Whether or not a collection sync is triggered on demand, ie. as soon
 *   as it is accessed by a client.
 * - An optional time interval for regular collection sync (aka interval
 *   mail check).
 *
 * @code
 *
 * Akonadi::CachePolicy policy;
 * policy.setCacheTimeout( 30 );
 * policy.setIntervalCheckTime( 20 );
 *
 * Akonadi::Collection collection = ...
 * collection.setCachePolicy( policy );
 *
 * @endcode
 *
 * @todo Do we also need a size limit for the cache as well?
 * @todo on a POP3 account, is should not be possible to change locally cached parts, find a solution for that
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADI_EXPORT CachePolicy
{
  public:
    /**
     * Creates an empty cache policy.
     */
    CachePolicy();

    /**
     * Creates a cache policy from an @p other cache policy.
     */
    CachePolicy( const CachePolicy &other );

    /**
     * Destroys the cache policy.
     */
    ~CachePolicy();

    /**
     * Returns whether it inherits cache policy from parent collection.
     */
    bool inheritFromParent() const;

    /**
     * Sets whether the cache policy should be inherited from the parent collection.
     */
    void setInheritFromParent( bool inherit );

    /**
     * Returns the parts to permanently cache locally.
     */
    QStringList localParts() const;

    /**
     * Specifies the parts to permanently cache locally.
     */
    void setLocalParts( const QStringList &parts );

    /**
     * Returns the cache timeout for non-permanently cached parts in minutes,
     * -1 means indefinitely.
     */
    int cacheTimeout() const;

    /**
     * Sets cache timeout for non-permanently cached parts.
     * @param timeout Timeout in minutes, -1 for indefinitely.
     */
    void setCacheTimeout( int timeout );

    /**
     * Returns the interval check time in minutes, -1 for never.
     */
    int intervalCheckTime() const;

    /**
     * Sets interval check time.
     * @param time Check time interval in minutes, -1 for never.
     */
    void setIntervalCheckTime( int time );

    /**
     * Returns whether the collection shall be synced automatically when necessary.
     */
    bool syncOnDemand() const;

    /**
     * Sets whether the collection shall be synced automatically when necessary.
     * @param enable If @c true the collection is synced.
     */
    void setSyncOnDemand( bool enable );

    /**
     * @internal.
     */
    CachePolicy& operator=( const CachePolicy &other );

    /**
     * @internal
     */
    bool operator==( const CachePolicy &other ) const;

  private:
    //@cond PRIVATE
    class Private;
    QSharedDataPointer<Private> d;
    //@endcond
};

}

/**
 * Allows to output a cache policy for debugging purposes.
 */
AKONADI_EXPORT QDebug operator<<( QDebug, const Akonadi::CachePolicy& );

#endif
