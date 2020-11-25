/*
    SPDX-FileCopyrightText: 2019 David Faure <faure@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "fakeintervalcheck.h"

using namespace Akonadi::Server;

FakeIntervalCheck::FakeIntervalCheck(ItemRetrievalManager &retrievalManager)
    : IntervalCheck(retrievalManager)
{
}

void FakeIntervalCheck::waitForInit()
{
    m_initCalled.acquire();
}

void FakeIntervalCheck::init()
{
    IntervalCheck::init();
    m_initCalled.release();
}

bool FakeIntervalCheck::shouldScheduleCollection(const Collection &collection)
{
    return (collection.syncPref() == Collection::True) || ((collection.syncPref() == Collection::Undefined) && collection.enabled());
}

bool FakeIntervalCheck::hasChanged(const Collection &collection, const Collection &changed)
{
    Q_ASSERT(collection.id() == changed.id());
    return collection.cachePolicyCheckInterval() != changed.cachePolicyCheckInterval();
}

int FakeIntervalCheck::collectionScheduleInterval(const Collection &collection)
{
    return collection.cachePolicyCheckInterval();
}

void FakeIntervalCheck::collectionExpired(const Collection &collection)
{
    Q_UNUSED(collection)
    // Nothing here. The granularity is in whole minutes, we don't have time to wait for that in a unittest.
}
