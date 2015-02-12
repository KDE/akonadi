/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_INVALIDATECACHEJOB_P_H
#define AKONADI_INVALIDATECACHEJOB_P_H

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi {

class Collection;
class InvalidateCacheJobPrivate;

/**
 * Helper job to invalidate item cache for an entire collection.
 * @since 4.8
 */
class AKONADICORE_EXPORT InvalidateCacheJob : public Akonadi::Job
{
    Q_OBJECT
public:
    /**
     * Create a job to invalidate all cached content in @p collection.
     */
    explicit InvalidateCacheJob(const Collection &collection, QObject *parent);

protected:
    void doStart() Q_DECL_OVERRIDE;

private:
    Q_DECLARE_PRIVATE(InvalidateCacheJob)
    Q_PRIVATE_SLOT(d_func(), void collectionFetchResult(KJob *job))
    Q_PRIVATE_SLOT(d_func(), void itemFetchResult(KJob *job))
    Q_PRIVATE_SLOT(d_func(), void itemStoreResult(KJob *job))
};

}

#endif // AKONADI_INVALIDATECACHEJOB_P_H
