/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2013 Daniel Vrátil <dvratil@redhat.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "searchmanager.h"
#include "abstractsearchplugin.h"
#include "akonadiserver_search_debug.h"

#include "agentsearchengine.h"
#include "akonadi.h"
#include "handler/searchhelper.h"
#include "notificationmanager.h"
#include "searchrequest.h"
#include "searchtaskmanager.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include "storage/selectquerybuilder.h"
#include "storage/transaction.h"

#include <private/protocol_p.h>

#include <QDBusConnection>
#include <QDir>
#include <QPluginLoader>
#include <QTimer>

#include <memory>

Q_DECLARE_METATYPE(Akonadi::Server::NotificationCollector *)

using namespace Akonadi;
using namespace Akonadi::Server;

Q_DECLARE_METATYPE(Collection)

SearchManager::SearchManager(const QStringList &searchEngines, SearchTaskManager &agentSearchManager)
    : AkThread(QStringLiteral("SearchManager"), AkThread::ManualStart, QThread::InheritPriority)
    , mAgentSearchManager(agentSearchManager)
    , mEngineNames(searchEngines)
    , mSearchUpdateTimer(nullptr)
{
    qRegisterMetaType<Collection>();

    // We load search plugins (as in QLibrary::load()) in the main thread so that
    // static initialization happens in the QApplication thread
    loadSearchPlugins();

    // Register to DBus on the main thread connection - otherwise we don't appear
    // on the service.
    QDBusConnection conn = QDBusConnection::sessionBus();
    conn.registerObject(QStringLiteral("/SearchManager"), this, QDBusConnection::ExportAllSlots);

    // Delay-call init()
    startThread();
}

void SearchManager::init()
{
    AkThread::init();

    mEngines.reserve(mEngineNames.size());
    for (const QString &engineName : std::as_const(mEngineNames)) {
        if (engineName == QLatin1String("Agent")) {
            mEngines.append(new AgentSearchEngine);
        } else {
            qCCritical(AKONADISERVER_SEARCH_LOG) << "Unknown search engine type: " << engineName;
        }
    }

    initSearchPlugins();

    // The timer will tick 15 seconds after last change notification. If a new notification
    // is delivered in the meantime, the timer is reset
    mSearchUpdateTimer = new QTimer(this);
    mSearchUpdateTimer->setInterval(15 * 1000);
    mSearchUpdateTimer->setSingleShot(true);
    connect(mSearchUpdateTimer, &QTimer::timeout, this, &SearchManager::searchUpdateTimeout);
}

void SearchManager::quit()
{
    QDBusConnection conn = QDBusConnection::sessionBus();
    conn.unregisterObject(QStringLiteral("/SearchManager"), QDBusConnection::UnregisterTree);
    conn.disconnectFromBus(conn.name());

    // Make sure all children are deleted within context of this thread
    qDeleteAll(children());

    qDeleteAll(mEngines);
    qDeleteAll(mPlugins);
    /*
     * FIXME: Unloading plugin messes up some global statics from client libs
     * and causes crash on Akonadi shutdown (below main). Keeping the plugins
     * loaded is not really a big issue as this is only invoked on server shutdown
     * anyway, so we are not leaking any memory.
    Q_FOREACH (QPluginLoader *loader, mPluginLoaders) {
        loader->unload();
        delete loader;
    }
    */

    AkThread::quit();
}

SearchManager::~SearchManager()
{
    quitThread();
}

void SearchManager::registerInstance(const QString &id)
{
    mAgentSearchManager.registerInstance(id);
}

void SearchManager::unregisterInstance(const QString &id)
{
    mAgentSearchManager.unregisterInstance(id);
}

QVector<AbstractSearchPlugin *> SearchManager::searchPlugins() const
{
    return mPlugins;
}

void SearchManager::loadSearchPlugins()
{
    QStringList loadedPlugins;
    const QString pluginOverride = QString::fromLatin1(qgetenv("AKONADI_OVERRIDE_SEARCHPLUGIN"));
    if (!pluginOverride.isEmpty()) {
        qCInfo(AKONADISERVER_SEARCH_LOG) << "Overriding the search plugins with: " << pluginOverride;
    }

    const QStringList dirs = QCoreApplication::libraryPaths();
    for (const QString &pluginDir : dirs) {
        QDir dir(pluginDir + QLatin1String("/akonadi"));
        const QStringList fileNames = dir.entryList(QDir::Files);
        qCDebug(AKONADISERVER_SEARCH_LOG) << "SEARCH MANAGER: searching in " << pluginDir + QLatin1String("/akonadi") << ":" << fileNames;
        for (const QString &fileName : fileNames) {
            const QString filePath = pluginDir % QLatin1String("/akonadi/") % fileName;
            std::unique_ptr<QPluginLoader> loader(new QPluginLoader(filePath));
            const QVariantMap metadata = loader->metaData().value(QStringLiteral("MetaData")).toVariant().toMap();
            if (metadata.value(QStringLiteral("X-Akonadi-PluginType")).toString() != QLatin1String("SearchPlugin")) {
                continue;
            }

            const QString libraryName = metadata.value(QStringLiteral("X-Akonadi-Library")).toString();
            if (loadedPlugins.contains(libraryName)) {
                qCDebug(AKONADISERVER_SEARCH_LOG) << "Already loaded one version of this plugin, skipping: " << libraryName;
                continue;
            }

            // When search plugin override is active, ignore all plugins except for the override
            if (!pluginOverride.isEmpty()) {
                if (libraryName != pluginOverride) {
                    qCDebug(AKONADISERVER_SEARCH_LOG) << libraryName << "skipped because of AKONADI_OVERRIDE_SEARCHPLUGIN";
                    continue;
                }

                // When there's no override, only load plugins enabled by default
            } else if (!metadata.value(QStringLiteral("X-Akonadi-LoadByDefault"), true).toBool()) {
                continue;
            }

            if (!loader->load()) {
                qCCritical(AKONADISERVER_SEARCH_LOG) << "Failed to load search plugin" << libraryName << ":" << loader->errorString();
                continue;
            }

            mPluginLoaders << loader.release();
            loadedPlugins << libraryName;
        }
    }
}

void SearchManager::initSearchPlugins()
{
    for (QPluginLoader *loader : std::as_const(mPluginLoaders)) {
        if (!loader->load()) {
            qCCritical(AKONADISERVER_SEARCH_LOG) << "Failed to load search plugin" << loader->fileName() << ":" << loader->errorString();
            continue;
        }

        AbstractSearchPlugin *plugin = qobject_cast<AbstractSearchPlugin *>(loader->instance());
        if (!plugin) {
            qCCritical(AKONADISERVER_SEARCH_LOG) << loader->fileName() << "is not a valid Akonadi search plugin";
            continue;
        }

        qCDebug(AKONADISERVER_SEARCH_LOG) << "SearchManager: loaded search plugin" << loader->fileName();
        mPlugins << plugin;
    }
}

void SearchManager::scheduleSearchUpdate()
{
    // Reset if the timer is active (use QueuedConnection to invoke start() from
    // the thread the QTimer lives in instead of caller's thread, otherwise crashes
    // and weird things can happen.
    QMetaObject::invokeMethod(mSearchUpdateTimer, QOverload<>::of(&QTimer::start), Qt::QueuedConnection);
}

void SearchManager::searchUpdateTimeout()
{
    // Get all search collections, that is subcollections of "Search", which always has ID 1
    const Collection::List collections = Collection::retrieveFiltered(Collection::parentIdFullColumnName(), 1);
    for (const Collection &collection : collections) {
        updateSearchAsync(collection);
    }
}

void SearchManager::updateSearchAsync(const Collection &collection)
{
    QMetaObject::invokeMethod(
        this,
        [this, collection]() {
            updateSearchImpl(collection);
        },
        Qt::QueuedConnection);
}

void SearchManager::updateSearch(const Collection &collection)
{
    mLock.lock();
    if (mUpdatingCollections.contains(collection.id())) {
        mLock.unlock();
        return;
        // FIXME: If another thread already requested an update, we return to the caller before the
        // search update is performed; this contradicts the docs
    }
    mUpdatingCollections.insert(collection.id());
    mLock.unlock();
    QMetaObject::invokeMethod(
        this,
        [this, collection]() {
            updateSearchImpl(collection);
        },
        Qt::BlockingQueuedConnection);
    mLock.lock();
    mUpdatingCollections.remove(collection.id());
    mLock.unlock();
}

void SearchManager::updateSearchImpl(const Collection &collection)
{
    if (collection.queryString().size() >= 32768) {
        qCWarning(AKONADISERVER_SEARCH_LOG) << "The query is at least 32768 chars long, which is the maximum size supported by the akonadi db schema. The "
                                               "query is therefore most likely truncated and will not be executed.";
        return;
    }
    if (collection.queryString().isEmpty()) {
        return;
    }

    const QStringList queryAttributes = collection.queryAttributes().split(QLatin1Char(' '));
    const bool remoteSearch = queryAttributes.contains(QLatin1String(AKONADI_PARAM_REMOTE));
    bool recursive = queryAttributes.contains(QLatin1String(AKONADI_PARAM_RECURSIVE));

    QStringList queryMimeTypes;
    const QVector<MimeType> mimeTypes = collection.mimeTypes();
    queryMimeTypes.reserve(mimeTypes.count());

    for (const MimeType &mt : mimeTypes) {
        queryMimeTypes << mt.name();
    }

    QVector<qint64> queryAncestors;
    if (collection.queryCollections().isEmpty()) {
        queryAncestors << 0;
        recursive = true;
    } else {
        const QStringList collectionIds = collection.queryCollections().split(QLatin1Char(' '));
        queryAncestors.reserve(collectionIds.count());
        for (const QString &colId : collectionIds) {
            queryAncestors << colId.toLongLong();
        }
    }

    // Always query the given collections
    QVector<qint64> queryCollections = queryAncestors;

    if (recursive) {
        // Resolve subcollections if necessary
        queryCollections += SearchHelper::matchSubcollectionsByMimeType(queryAncestors, queryMimeTypes);
    }

    // This happens if we try to search a virtual collection in recursive mode (because virtual collections are excluded from listCollectionsRecursive)
    if (queryCollections.isEmpty()) {
        qCDebug(AKONADISERVER_SEARCH_LOG) << "No collections to search, you're probably trying to search a virtual collection.";
        return;
    }

    // Query all plugins for search results
    const QByteArray id = "searchUpdate-" + QByteArray::number(QDateTime::currentDateTimeUtc().toTime_t());
    SearchRequest request(id, *this, mAgentSearchManager);
    request.setCollections(queryCollections);
    request.setMimeTypes(queryMimeTypes);
    request.setQuery(collection.queryString());
    request.setRemoteSearch(remoteSearch);
    request.setStoreResults(true);
    request.setProperty("SearchCollection", QVariant::fromValue(collection));
    connect(&request, &SearchRequest::resultsAvailable, this, &SearchManager::searchUpdateResultsAvailable);
    request.exec(); // blocks until all searches are done

    const QSet<qint64> results = request.results();

    // Get all items in the collection
    QueryBuilder qb(CollectionPimItemRelation::tableName());
    qb.addColumn(CollectionPimItemRelation::rightColumn());
    qb.addValueCondition(CollectionPimItemRelation::leftColumn(), Query::Equals, collection.id());
    if (!qb.exec()) {
        return;
    }

    Transaction transaction(DataStore::self(), QStringLiteral("UPDATE SEARCH"));

    // Unlink all items that were not in search results from the collection
    QVariantList toRemove;
    while (qb.query().next()) {
        const qint64 id = qb.query().value(0).toLongLong();
        if (!results.contains(id)) {
            toRemove << id;
            Collection::removePimItem(collection.id(), id);
        }
    }

    if (!transaction.commit()) {
        return;
    }

    if (!toRemove.isEmpty()) {
        SelectQueryBuilder<PimItem> qb;
        qb.addValueCondition(PimItem::idFullColumnName(), Query::In, toRemove);
        if (!qb.exec()) {
            return;
        }

        const QVector<PimItem> removedItems = qb.result();
        DataStore::self()->notificationCollector()->itemsUnlinked(removedItems, collection);
    }

    qCInfo(AKONADISERVER_SEARCH_LOG) << "Search update for collection" << collection.name() << "(" << collection.id() << ") finished:"
                                     << "all results: " << results.count() << ", removed results:" << toRemove.count();
}

void SearchManager::searchUpdateResultsAvailable(const QSet<qint64> &results)
{
    const auto collection = sender()->property("SearchCollection").value<Collection>();
    qCDebug(AKONADISERVER_SEARCH_LOG) << "searchUpdateResultsAvailable" << collection.id() << results.count() << "results";

    QSet<qint64> newMatches = results;
    QSet<qint64> existingMatches;
    {
        QueryBuilder qb(CollectionPimItemRelation::tableName());
        qb.addColumn(CollectionPimItemRelation::rightColumn());
        qb.addValueCondition(CollectionPimItemRelation::leftColumn(), Query::Equals, collection.id());
        if (!qb.exec()) {
            return;
        }

        while (qb.query().next()) {
            const qint64 id = qb.query().value(0).toLongLong();
            if (newMatches.contains(id)) {
                existingMatches << id;
            }
        }
    }

    qCDebug(AKONADISERVER_SEARCH_LOG) << "Got" << newMatches.count() << "results, out of which" << existingMatches.count() << "are already in the collection";

    newMatches = newMatches - existingMatches;
    if (newMatches.isEmpty()) {
        qCDebug(AKONADISERVER_SEARCH_LOG) << "Added results: 0 (fast path)";
        return;
    }

    Transaction transaction(DataStore::self(), QStringLiteral("PUSH SEARCH RESULTS"), !DataStore::self()->inTransaction());

    // First query all the IDs we got from search plugin/agent against the DB.
    // This will remove IDs that no longer exist in the DB.
    QVariantList newMatchesVariant;
    newMatchesVariant.reserve(newMatches.count());
    for (qint64 id : std::as_const(newMatches)) {
        newMatchesVariant << id;
    }

    SelectQueryBuilder<PimItem> qb;
    qb.addValueCondition(PimItem::idFullColumnName(), Query::In, newMatchesVariant);
    if (!qb.exec()) {
        return;
    }

    const auto items = qb.result();
    if (items.count() != newMatches.count()) {
        qCDebug(AKONADISERVER_SEARCH_LOG) << "Search backend returned" << (newMatches.count() - items.count()) << "results that no longer exist in Akonadi.";
        qCDebug(AKONADISERVER_SEARCH_LOG) << "Please reindex collection" << collection.id();
        // TODO: Request the reindexing directly from here
    }

    if (items.isEmpty()) {
        qCDebug(AKONADISERVER_SEARCH_LOG) << "Added results: 0 (no existing result)";
        return;
    }

    for (const auto &item : items) {
        Collection::addPimItem(collection.id(), item.id());
    }

    if (!transaction.commit()) {
        qCWarning(AKONADISERVER_SEARCH_LOG) << "Failed to commit search results transaction";
        return;
    }

    DataStore::self()->notificationCollector()->itemsLinked(items, collection);
    // Force collector to dispatch the notification now
    DataStore::self()->notificationCollector()->dispatchNotifications();

    qCDebug(AKONADISERVER_SEARCH_LOG) << "Added results:" << items.count();
}
