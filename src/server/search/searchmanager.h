/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef SEARCHMANAGER_H
#define SEARCHMANAGER_H

#include "akthread.h"

#include <QVector>
#include <QSet>
#include <QMutex>
#include <QDBusConnection>

class QSemaphore;
class QTimer;
class QPluginLoader;

namespace Akonadi
{

class AbstractSearchPlugin;

namespace Server
{

class AbstractSearchEngine;
class Collection;

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
    explicit SearchManager(const QStringList &searchEngines, QObject *parent = Q_NULLPTR);

    ~SearchManager();

    /**
     * Returns a global instance of the search manager.
     */
    static SearchManager *instance();

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
     * Since caller invokes this method from a different thread, they use
     * QMetaObject::invokeMethod(). To still make it possible for callers to behave
     * synchronously, we can pass in a QWaitCondition that the code will wake up
     * once the search update is completed.
     */
    void updateSearchImpl(const Collection &collection, QSemaphore *cond);

private:
    void init() Q_DECL_OVERRIDE;
    void quit() Q_DECL_OVERRIDE;

    // Called from main thread
    void loadSearchPlugins();
    // Called from manager thread
    void initSearchPlugins();

    static SearchManager *sInstance;

    QStringList mEngineNames;
    QVector<QPluginLoader *> mPluginLoaders;
    QVector<AbstractSearchEngine *> mEngines;
    QVector<AbstractSearchPlugin *> mPlugins;

    QTimer *mSearchUpdateTimer;

    QMutex mLock;
    QSet<qint64> mUpdatingCollections;

};

} // namespace Server
} // namespace Akonadi

#endif
