/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "collectionscheduler.h"

#include <QDateTime>
#include <QHash>

namespace Akonadi
{
namespace Server
{
class ItemRetrievalManager;

/**
  Interval checking thread.
*/
class IntervalCheck : public CollectionScheduler
{
    Q_OBJECT

protected:
    /**
     * Use AkThread::create() to create and start a new IntervalCheck thread.
     */
    explicit IntervalCheck(ItemRetrievalManager &itemRetrievalManager);

public:
    ~IntervalCheck() override;

    /**
     * Requests the given collection to be synced.
     * Executed from any thread, forwards to triggerCollectionXSync() in the
     * retrieval thread.
     * A minimum time interval between two sync requests is ensured.
     */
    void requestCollectionSync(const Collection &collection);

protected:
    int collectionScheduleInterval(const Collection &collection) override;
    bool hasChanged(const Collection &collection, const Collection &changed) override;
    bool shouldScheduleCollection(const Collection &collection) override;

    void collectionExpired(const Collection &collection) override;

private:
    QHash<int, QDateTime> mLastChecks;
    QHash<QString, QDateTime> mLastCollectionTreeSyncs;
    ItemRetrievalManager &mItemRetrievalManager;
};

} // namespace Server
} // namespace Akonadi
