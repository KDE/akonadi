/*
    Copyright (c) 2019 David Faure <faure@kde.org>

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

#include "fakecollectionscheduler.h"

using namespace Akonadi::Server;

FakeCollectionScheduler::FakeCollectionScheduler(QObject *parent)
    : CollectionScheduler(QStringLiteral("FakeCollectionScheduler"), QThread::NormalPriority, parent)
{
}

void FakeCollectionScheduler::waitForInit()
{
    m_initCalled.acquire();
}

void FakeCollectionScheduler::init()
{
    CollectionScheduler::init();
    m_initCalled.release();
}

bool FakeCollectionScheduler::hasChanged(const Collection &collection, const Collection &changed)
{
    Q_ASSERT(collection.id() == changed.id());
    return collection.cachePolicyCheckInterval() != changed.cachePolicyCheckInterval();
}

int FakeCollectionScheduler::collectionScheduleInterval(const Collection &collection)
{
    return collection.cachePolicyCheckInterval();
}

void FakeCollectionScheduler::collectionExpired(const Collection &collection)
{
    Q_UNUSED(collection);
    // Nothing here. The granularity is in whole minutes, we don't have time to wait for that in a unittest.
}
