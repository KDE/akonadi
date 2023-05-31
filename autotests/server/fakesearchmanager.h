/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#pragma once

#include <search/searchmanager.h>

namespace Akonadi
{
namespace Server
{
class SearchTaskManager;

/**
 * Subclass of SearchManager that does nothing.
 */
class FakeSearchManager : public SearchManager
{
    Q_OBJECT

public:
    explicit FakeSearchManager(SearchTaskManager &searchTaskManager);
    ~FakeSearchManager() override;

    void registerInstance(const QString &id) override;
    void unregisterInstance(const QString &id) override;
    void updateSearch(const Collection &collection) override;
    void updateSearchAsync(const Collection &collection) override;
    QList<AbstractSearchPlugin *> searchPlugins() const override;

    void scheduleSearchUpdate() override;
};

} // namespace Server
} // namespace Akonadi
