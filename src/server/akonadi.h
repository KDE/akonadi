/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>                 *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#ifndef AKONADISERVER_H
#define AKONADISERVER_H

#include <QObject>
#include <QVector>

#include <memory>

class QDBusServiceWatcher;
class QSettings;

namespace Akonadi
{
namespace Server
{

class Connection;
class ItemRetrievalManager;
class SearchTaskManager;
class SearchManager;
class StorageJanitor;
class CacheCleaner;
class IntervalCheck;
class AkLocalServer;
class NotificationManager;
class ResourceManager;
class CollectionStatistics;
class PreprocessorManager;
class Tracer;
class DebugInterface;

class AkonadiServer : public QObject
{
    Q_OBJECT

public:
    explicit AkonadiServer();
    ~AkonadiServer();

    /**
     * Can return a nullptr
     */
    CacheCleaner *cacheCleaner();

    /**
     * Returns the IntervalCheck instance. Never nullptr.
     */
    IntervalCheck &intervalChecker();

    ResourceManager &resourceManager();

    CollectionStatistics &collectionStatistics();

    PreprocessorManager &preprocessorManager();

    SearchTaskManager &agentSearchManager();

    SearchManager &searchManager();

    ItemRetrievalManager &itemRetrievalManager();

    Tracer &tracer();

    /**
     * Instance-aware server .config directory
     */
    QString serverPath() const;

    /**
     * Can return a nullptr
     */
    NotificationManager *notificationManager();

public Q_SLOTS:
    /**
     * Triggers a clean server shutdown.
     */
    virtual bool quit();

    virtual bool init();

protected Q_SLOTS:
    virtual void newCmdConnection(quintptr socketDescriptor);

private Q_SLOTS:
    void doQuit();
    void connectionDisconnected();

private:
    bool startDatabaseProcess();
    bool createDatabase();
    void stopDatabaseProcess();
    bool createServers(QSettings &settings, QSettings &connectionSettings);
    bool setupDatabase();
    uint userId() const;

protected:
    std::unique_ptr<QDBusServiceWatcher> mControlWatcher;

    std::unique_ptr<AkLocalServer> mCmdServer;
    std::unique_ptr<AkLocalServer> mNtfServer;

    std::unique_ptr<ResourceManager> mResourceManager;
    std::unique_ptr<DebugInterface> mDebugInterface;
    std::unique_ptr<CollectionStatistics> mCollectionStats;
    std::unique_ptr<PreprocessorManager> mPreprocessorManager;
    std::unique_ptr<NotificationManager> mNotificationManager;
    std::unique_ptr<CacheCleaner> mCacheCleaner;
    std::unique_ptr<IntervalCheck> mIntervalCheck;
    std::unique_ptr<StorageJanitor> mStorageJanitor;
    std::unique_ptr<ItemRetrievalManager> mItemRetrieval;
    std::unique_ptr<SearchTaskManager> mAgentSearchManager;
    std::unique_ptr<SearchManager> mSearchManager;
    std::unique_ptr<Tracer> mTracer;

    std::vector<std::unique_ptr<Connection>> mConnections;
    bool mAlreadyShutdown = false;
};

} // namespace Server
} // namespace Akonadi
#endif
