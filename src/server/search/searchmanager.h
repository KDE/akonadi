/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akthread.h"

#include <QMutex>
#include <QSet>
#include <QVector>

class QTimer;
class QPluginLoader;

namespace Akonadi
{
class AbstractSearchPlugin;

namespace Server
{
class AbstractSearchEngine;
class Collection;
class SearchTaskManager;

/**
 * SearchManager creates and deletes persistent searches for all currently
 * active search engines.
 */
class SearchManager : public AkThread
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.SearchManager")

public:
    /** Create a new search manager with the given @p searchEngines. */
    explicit SearchManager(const QStringList &searchEngines, SearchTaskManager &agentSearchManager);

    ~SearchManager() override;

    /**
     * Updates the search query asynchronously. Returns immediately
     */
    virtual void updateSearchAsync(const Collection &collection);

    /**
     * Updates the search query synchronously.
     */
    virtual void updateSearch(const Collection &collection);

    /**
     * Returns currently available search plugins.
     */
    virtual QVector<AbstractSearchPlugin *> searchPlugins() const;

public Q_SLOTS:
    virtual void scheduleSearchUpdate();

    /**
     * This is called via D-Bus from AgentManager to register an agent with
     * search interface.
     */
    virtual void registerInstance(const QString &id);

    /**
     * This is called via D-Bus from AgentManager to unregister an agent with
     * search interface.
     */
    virtual void unregisterInstance(const QString &id);

private Q_SLOTS:
    void searchUpdateTimeout();
    void searchUpdateResultsAvailable(const QSet<qint64> &results);

    /**
     * Actual implementation of search updates.
     *
     * This method has to be called using QMetaObject::invokeMethod.
     */
    void updateSearchImpl(const Akonadi::Server::Collection &collection);

private:
    void init() override;
    void quit() override;

    // Called from main thread
    void loadSearchPlugins();
    // Called from manager thread
    void initSearchPlugins();

    SearchTaskManager &mAgentSearchManager;
    QStringList mEngineNames;
    QVector<QPluginLoader *> mPluginLoaders;
    QVector<AbstractSearchEngine *> mEngines;
    QVector<AbstractSearchPlugin *> mPlugins;

    QTimer *mSearchUpdateTimer = nullptr;

    QMutex mLock;
    QSet<qint64> mUpdatingCollections;
};

} // namespace Server
} // namespace Akonadi
