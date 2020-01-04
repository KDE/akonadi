/*
 * Copyright (C) 2014  Daniel Vrátil <dvratil@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef AKONADI_SERVER_FAKESEARCHMANAGER_H
#define AKONADI_SERVER_FAKESEARCHMANAGER_H

#include <search/searchmanager.h>

namespace Akonadi {
namespace Server {

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
    QVector<AbstractSearchPlugin *> searchPlugins() const override;

    void scheduleSearchUpdate() override;
};

} // namespace Server
} // namespace Akonadi

#endif // AKONADI_SERVER_FAKESEARCHMANAGER_H
