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

#include "libakonadi_export.h"

#include <QtCore/QSharedDataPointer>
#include <QtCore/QStringList>

namespace Akonadi {

/**
  Represents the caching policy for a collection.

  There is one cache policy per collection, it can either define to
  inherit all properties of the policy of the parent collection (the default)
  or specify the following values:

- The item parts that should be permanently kept locally and are downloaded
  during a collection sync (eg. full mail vs. just the headers).
- A time up to which non-permantly cached item parts have to be kept at
  least (0 - infinity).
- Whether or not a collection sync is triggered on demand, ie. as soon
  as it is accessed by a client.
- An optional time interval for regular collection sync (aka interval
  mail check).

  This class is implicitly shared.

  @todo Do we also need a size limit for the cache as well?
  @todo on a POP3 account, is should not be possible to change locally cached parts, find a solution for that
*/
class AKONADI_EXPORT CachePolicy
{
  public:
    /**
      Create a empty cache policy.
    */
    CachePolicy();

    /**
      Copy constructor.
    */
    CachePolicy( const CachePolicy &other );

    /**
      Destructor.
    */
    ~CachePolicy();

    /**
      Assignement operator.
    */
    CachePolicy& operator=( const CachePolicy &other );

    /**
      Comparisson operator
    */
    bool operator==( const CachePolicy &other ) const;

    /**
      Inherit cache policy from parent collection.
    */
    bool inheritFromParent() const;

    /**
      Set if the cache policy should be inherited from the parent collection.
    */
    void setInheritFromParent( bool inherit );

    /**
      Parts to permanently cache locally.
    */
    QStringList localParts() const;

    /**
      Specify the parts to permanently cache locally.
    */
    void setLocalParts( const QStringList &parts );

    /**
      Cache timeout for non-permanently cached parts in minutes,
      -1 means indefinitely.
    */
    int cacheTimeout() const;

    /**
      Set cache timeout for non-permanently cached parts.
      @param timeout Timeout in minutes, -1 for indefinitely.
    */
    void setCacheTimeout( int timeout );

    /**
      Interval check time in minutes, -1 for never.
    */
    int intervalCheckTime() const;

    /**
      Set interval check time.
      @param time Check time interval in minutes, -1 for never.
    */
    void setIntervalCheckTime( int time );

    /**
      Sync collection automatically on every access.
    */
    bool syncOnDemand() const;

    /**
      Enable on-demand syncing.
    */
    void enableSyncOnDemand( bool enable );

  private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

#endif
