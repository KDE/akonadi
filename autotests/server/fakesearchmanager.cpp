/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "fakesearchmanager.h"

#include "entities.h"

using namespace Akonadi::Server;

FakeSearchManager::FakeSearchManager(SearchTaskManager &agentSearchManager)
    : SearchManager(QStringList(), agentSearchManager)
{
}

FakeSearchManager::~FakeSearchManager()
{
}

void FakeSearchManager::registerInstance(const QString &id)
{
    Q_UNUSED(id);
}

void FakeSearchManager::unregisterInstance(const QString &id)
{
    Q_UNUSED(id);
}

void FakeSearchManager::updateSearch(const Collection &collection)
{
    Q_UNUSED(collection);
}

void FakeSearchManager::updateSearchAsync(const Collection &collection)
{
    Q_UNUSED(collection);
}

QVector<Akonadi::AbstractSearchPlugin *> FakeSearchManager::searchPlugins() const
{
    return QVector<Akonadi::AbstractSearchPlugin *>();
}

void FakeSearchManager::scheduleSearchUpdate()
{
}
