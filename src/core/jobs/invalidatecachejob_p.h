/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadicore_export.h"
#include "job.h"

namespace Akonadi
{
class Collection;
class InvalidateCacheJobPrivate;

/*!
 * Helper job to invalidate item cache for an entire collection.
 *
 * \class Akonadi::InvalidateCacheJob
 * \inheaderfile Akonadi/InvalidateCacheJob
 * \inmodule AkonadiCore
 *
 * \since 4.8
 */
class AKONADICORE_EXPORT InvalidateCacheJob : public Akonadi::Job
{
    Q_OBJECT
public:
    /*!
     * Create a job to invalidate all cached content in \ collection.
     */
    explicit InvalidateCacheJob(const Collection &collection, QObject *parent);

protected:
    void doStart() override;

private:
    Q_DECLARE_PRIVATE(InvalidateCacheJob)
};

}
