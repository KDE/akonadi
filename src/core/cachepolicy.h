/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"

#include <QSharedDataPointer>
#include <QStringList>

namespace Akonadi
{
class CachePolicyPrivate;

/**
 * @short Represents the caching policy for a collection.
 *
 * There is one cache policy per collection. It can either specify that all
 * properties of the policy of the parent collection will be inherited (the
 * default) or specify the following values:
 *
 * - The item parts that should be permanently kept locally and are downloaded
 *   during a collection sync (e.g. full mail vs. just the headers).
 * - A minimum time for which non-permanently cached item parts have to be kept
 *   (0 - infinity).
 * - Whether or not a collection sync is triggered on demand, i.e. as soon
 *   as it is accessed by a client.
 * - An optional time interval for regular collection sync (aka interval
 *   mail check).
 *
 * Syncing means fetching updates from the Akonadi database. The cache policy
 * does not affect updates of the Akonadi database from the backend, since
 * backend updates will normally immediately trigger the resource to update the
 * Akonadi database.
 *
 * The cache policy applies only to reading from the collection. Writing to the
 * collection is independent of cache policy - all updates are written to the
 * backend as soon as the resource can schedule this.
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
class AKONADICORE_EXPORT CachePolicy
{
public:
    /**
     * Creates an empty cache policy.
     */
    CachePolicy();

    /**
     * Creates a cache policy from an @p other cache policy.
     */
    CachePolicy(const CachePolicy &other);

    /**
     * Destroys the cache policy.
     */
    ~CachePolicy();

    /**
     * Returns whether it inherits cache policy from the parent collection.
     */
    bool inheritFromParent() const;

    /**
     * Sets whether the cache policy should be inherited from the parent collection.
     */
    void setInheritFromParent(bool inherit);

    /**
     * Returns the parts to permanently cache locally.
     */
    [[nodiscard]] QStringList localParts() const;

    /**
     * Specifies the parts to permanently cache locally.
     */
    void setLocalParts(const QStringList &parts);

    /**
     * Returns the cache timeout for non-permanently cached parts in minutes;
     * -1 means indefinitely.
     */
    [[nodiscard]] int cacheTimeout() const;

    /**
     * Sets cache timeout for non-permanently cached parts.
     * @param timeout Timeout in minutes, -1 for indefinitely.
     */
    void setCacheTimeout(int timeout);

    /**
     * Returns the interval check time in minutes, -1 for never.
     */
    [[nodiscard]] int intervalCheckTime() const;

    /**
     * Sets interval check time.
     * @param time Check time interval in minutes, -1 for never.
     */
    void setIntervalCheckTime(int time);

    /**
     * Returns whether the collection will be synced automatically when necessary,
     * i.e. as soon as it is accessed by a client.
     */
    [[nodiscard]] bool syncOnDemand() const;

    /**
     * Sets whether the collection shall be synced automatically when necessary,
     * i.e. as soon as it is accessed by a client.
     * @param enable If @c true the collection is synced.
     */
    void setSyncOnDemand(bool enable);

    /**
     * @internal.
     * @param other other cache policy
     */
    CachePolicy &operator=(const CachePolicy &other);

    /**
     * @internal
     * @param other other cache policy
     */
    [[nodiscard]] bool operator==(const CachePolicy &other) const;

private:
    /// @cond PRIVATE
    QSharedDataPointer<CachePolicyPrivate> d;
    /// @endcond
};

}

/**
 * Allows a cache policy to be output for debugging purposes.
 */
AKONADICORE_EXPORT QDebug operator<<(QDebug, const Akonadi::CachePolicy &);
