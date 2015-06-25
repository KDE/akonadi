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

#include "searchmanager.h"
#include "abstractsearchplugin.h"
#include "searchmanageradaptor.h"
#include "akonadiserver_debug.h"

#include "agentsearchengine.h"
#include "notificationmanager.h"
#include "dbusconnectionpool.h"
#include "searchrequest.h"
#include "searchtaskmanager.h"
#include "storage/datastore.h"
#include "storage/querybuilder.h"
#include "storage/transaction.h"
#include "storage/selectquerybuilder.h"
#include "handler/searchhelper.h"

#include <shared/akdebug.h>
#include <private/xdgbasedirs_p.h>
#include <private/protocol_p.h>

#include <QDir>
#include <QPluginLoader>
#include <QDBusConnection>
#include <QTimer>
#include <QSemaphore>

#include <memory>

Q_DECLARE_METATYPE(Akonadi::Server::NotificationCollector *)

using namespace Akonadi;
using namespace Akonadi::Server;

SearchManager *SearchManager::sInstance = 0;

Q_DECLARE_METATYPE(Collection)
Q_DECLARE_METATYPE(QSet<qint64>)
Q_DECLARE_METATYPE(QSemaphore *)

SearchManagerThread::SearchManagerThread(const QStringList &searchEngines, QObject *parent)
    : QThread(parent)
    , mSearchEngines(searchEngines)
{
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());

    // We load search plugins (as in QLibrary::load()) in the main thread so that
    // static initialization happens in the QApplication thread
    loadSearchPlugins();
}

SearchManagerThread::~SearchManagerThread()
{
    qDeleteAll(mPluginLoaders);
}

void SearchManagerThread::loadSearchPlugins()
{
    QStringList loadedPlugins;
    const QString pluginOverride = QString::fromLatin1(qgetenv("AKONADI_OVERRIDE_SEARCHPLUGIN"));
    if (!pluginOverride.isEmpty()) {
        akDebug() << "Overriding the search plugins with: " << pluginOverride;
    }

    const QStringList dirs = XdgBaseDirs::findPluginDirs();
    Q_FOREACH (const QString &pluginDir, dirs) {
        QDir dir(pluginDir + QLatin1String("/akonadi"));
        const QStringList fileNames = dir.entryList(QDir::Files);
        akDebug() << "SEARCH MANAGER: searching in " << pluginDir + QLatin1String("/akonadi") << ":" << fileNames;
        Q_FOREACH (const QString &fileName, fileNames) {
            const QString filePath = pluginDir % QLatin1String("/akonadi/") % fileName;
            std::unique_ptr<QPluginLoader> loader(new QPluginLoader(filePath));
            const QVariantMap metadata = loader->metaData().value(QLatin1String("MetaData")).toVariant().toMap();

            if (metadata.value(QLatin1String("X-Akonadi-PluginType")).toString() != QLatin1String("SearchPlugin")) {
                continue;
            }

            const QString libraryName = metadata.value(QLatin1String("X-Akonadi-Library")).toString();
            if (loadedPlugins.contains(libraryName)) {
                akDebug() << "Already loaded one version of this plugin, skipping: " << libraryName;
                continue;
            }

            // When search plugin override is active, ignore all plugins except for the override
            if (!pluginOverride.isEmpty()) {
                if (libraryName != pluginOverride) {
                    akDebug() << libraryName << "skipped because of AKONADI_OVERRIDE_SEARCHPLUGIN";
                    continue;
                }

            // When there's no override, only load plugins enabled by default
            } else if (metadata.value(QLatin1String("X-Akonadi-LoadByDefault"), true).toBool() == false) {
                continue;
            }

            if (!loader->load()) {
                akError() << "Failed to load search plugin" << libraryName << ":" << loader->errorString();
                continue;
            }

            mPluginLoaders << loader.release();
            loadedPlugins << libraryName;
        }
    }
}


void SearchManagerThread::run()
{
    SearchManager *manager = new SearchManager();
    manager->init(mSearchEngines, mPluginLoaders);
    exec();
    delete manager;
}

SearchManager::SearchManager(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType< QSet<qint64> >();
    qRegisterMetaType<Collection>();
    qRegisterMetaType<QSemaphore *>();

    Q_ASSERT(sInstance == 0);
    sInstance = this;

    DataStore::self();
}

void SearchManager::init(const QStringList &searchEngines, const QList<QPluginLoader*> &loaders)
{
    mEngines.reserve(searchEngines.size());
    Q_FOREACH (const QString &engineName, searchEngines) {
        if (engineName == QLatin1String("Agent")) {
            mEngines.append(new AgentSearchEngine);
        } else {
            akError() << "Unknown search engine type: " << engineName;
        }
    }

    initSearchPlugins(loaders);

    new SearchManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(
        QLatin1String("/SearchManager"),
        this,
        QDBusConnection::ExportAdaptors);

    // The timer will tick 15 seconds after last change notification. If a new notification
    // is delivered in the meantime, the timer is reset
    mSearchUpdateTimer = new QTimer(this);
    mSearchUpdateTimer->setInterval(15 * 1000);
    mSearchUpdateTimer->setSingleShot(true);
    connect(mSearchUpdateTimer, SIGNAL(timeout()),
            this, SLOT(searchUpdateTimeout()));
}

SearchManager::~SearchManager()
{
    qDeleteAll(mEngines);
    DataStore::self()->close();
    sInstance = 0;
}

SearchManager *SearchManager::instance()
{
    Q_ASSERT(sInstance);
    return sInstance;
}

void SearchManager::registerInstance(const QString &id)
{
    SearchTaskManager::instance()->registerInstance(id);
}

void SearchManager::unregisterInstance(const QString &id)
{
    SearchTaskManager::instance()->unregisterInstance(id);
}

QVector<AbstractSearchPlugin *> SearchManager::searchPlugins() const
{
    return mPlugins;
}

void SearchManager::initSearchPlugins(const QList<QPluginLoader*> &loaders)
{
    for (QPluginLoader *loader : loaders) {
        if (!loader->load()) {
            akError() << "Failed to load search plugin" << loader->fileName() << ":" << loader->errorString();
            continue;
        }

        AbstractSearchPlugin *plugin = qobject_cast<AbstractSearchPlugin *>(loader->instance());
        if (!plugin) {
            akError() << loader->fileName() << "is not a valid Akonadi search plugin";
            continue;
        }

        akDebug() << "SearchManager: loaded search plugin" << loader->fileName();
        mPlugins << plugin;
    }
}

void SearchManager::scheduleSearchUpdate()
{
    // Reset if the timer is active (use QueuedConnection to invoke start() from
    // the thread the QTimer lives in instead of caller's thread, otherwise crashes
    // and weird things can happen.
    QMetaObject::invokeMethod(mSearchUpdateTimer, "start", Qt::QueuedConnection);
}

void SearchManager::searchUpdateTimeout()
{
    // Get all search collections, that is subcollections of "Search", which always has ID 1
    const Collection::List collections = Collection::retrieveFiltered(Collection::parentIdFullColumnName(), 1);
    Q_FOREACH (const Collection &collection, collections) {
        updateSearchAsync(collection);
    }
}

void SearchManager::updateSearchAsync(const Collection &collection)
{
    QMetaObject::invokeMethod(this, "updateSearchImpl",
                              Qt::QueuedConnection,
                              Q_ARG(Collection, collection),
                              Q_ARG(QSemaphore *, 0));
}

void SearchManager::updateSearch(const Collection &collection)
{
    QMutex mutex;

    mLock.lock();
    if (mUpdatingCollections.contains(collection.id())) {
        mLock.unlock();
        return;
    }
    mUpdatingCollections.insert(collection.id());
    mLock.unlock();

    QSemaphore sem(1);
    sem.acquire();

    QMetaObject::invokeMethod(this, "updateSearchImpl",
                              Qt::QueuedConnection,
                              Q_ARG(Collection, collection),
                              Q_ARG(QSemaphore*, &sem));

    // Now wait for updateSearchImpl to wake us.
    if (!sem.tryAcquire()) {
        sem.acquire();
    }
    sem.release();

    mLock.lock();
    mUpdatingCollections.remove(collection.id());
    mLock.unlock();
}

#define wakeUpCaller(cond) \
  if (cond) { \
    cond->release(); \
  }

void SearchManager::updateSearchImpl(const Collection &collection, QSemaphore *cond)
{
    if (collection.queryString().size() >= 32768) {
        qCWarning(AKONADISERVER_LOG) << "The query is at least 32768 chars long, which is the maximum size supported by the akonadi db schema. The query is therefore most likely truncated and will not be executed.";
        wakeUpCaller(cond);
        return;
    }
    if (collection.queryString().isEmpty()) {
        wakeUpCaller(cond);
        return;
    }

    const QStringList queryAttributes = collection.queryAttributes().split(QLatin1Char(' '));
    const bool remoteSearch =  queryAttributes.contains(QLatin1String(AKONADI_PARAM_REMOTE));
    bool recursive = queryAttributes.contains(QLatin1String(AKONADI_PARAM_RECURSIVE));

    QStringList queryMimeTypes;
    Q_FOREACH (const MimeType &mt, collection.mimeTypes()) {
        queryMimeTypes << mt.name();
    }

    QVector<qint64> queryCollections, queryAncestors;
    if (collection.queryCollections().isEmpty()) {
        queryAncestors << 0;
        recursive = true;
    } else {
        const QStringList collectionIds = collection.queryCollections().split(QLatin1Char(' '));
        queryAncestors.reserve(collectionIds.count());
        Q_FOREACH (const QString &colId, collectionIds) {
            queryAncestors << colId.toLongLong();
        }
    }

    if (recursive) {
        queryCollections = SearchHelper::matchSubcollectionsByMimeType(queryAncestors, queryMimeTypes);
    } else {
        queryCollections = queryAncestors;
    }

    //This happens if we try to search a virtual collection in recursive mode (because virtual collections are excluded from listCollectionsRecursive)
    if (queryCollections.isEmpty()) {
        akDebug() << "No collections to search, you're probably trying to search a virtual collection.";
        wakeUpCaller(cond);
        return;
    }

    // Query all plugins for search results
    SearchRequest request("searchUpdate-" + QByteArray::number(QDateTime::currentDateTime().toTime_t()));
    request.setCollections(queryCollections);
    request.setMimeTypes(queryMimeTypes);
    request.setQuery(collection.queryString());
    request.setRemoteSearch(remoteSearch);
    request.setStoreResults(true);
    request.setProperty("SearchCollection", QVariant::fromValue(collection));
    connect(&request, SIGNAL(resultsAvailable(QSet<qint64>)),
            this, SLOT(searchUpdateResultsAvailable(QSet<qint64>)));
    request.exec(); // blocks until all searches are done

    const QSet<qint64> results = request.results();

    // Get all items in the collection
    QueryBuilder qb(CollectionPimItemRelation::tableName());
    qb.addColumn(CollectionPimItemRelation::rightColumn());
    qb.addValueCondition(CollectionPimItemRelation::leftColumn(), Query::Equals, collection.id());
    if (!qb.exec()) {
        wakeUpCaller(cond);
        return;
    }

    DataStore::self()->beginTransaction();

    // Unlink all items that were not in search results from the collection
    QVariantList toRemove;
    while (qb.query().next()) {
        const qint64 id = qb.query().value(0).toLongLong();
        if (!results.contains(id)) {
            toRemove << id;
            Collection::removePimItem(collection.id(), id);
        }
    }

    if (!DataStore::self()->commitTransaction()) {
        wakeUpCaller(cond);
        return;
    }

    if (!toRemove.isEmpty()) {
        SelectQueryBuilder<PimItem> qb;
        qb.addValueCondition(PimItem::idFullColumnName(), Query::In, toRemove);
        if (!qb.exec()) {
            wakeUpCaller(cond);
            return;
        }

        const QVector<PimItem> removedItems = qb.result();
        DataStore::self()->notificationCollector()->itemsUnlinked(removedItems, collection);
    }

    akDebug() << "Search update finished";
    akDebug() << "All results:" << results.count();
    akDebug() << "Removed results:" << toRemove.count();

    wakeUpCaller(cond);
}

void SearchManager::searchUpdateResultsAvailable(const QSet<qint64> &results)
{
    const Collection collection = sender()->property("SearchCollection").value<Collection>();
    akDebug() << "searchUpdateResultsAvailable" << collection.id() << results.count() << "results";

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

    akDebug() << "Got" << newMatches.count() << "results, out of which" << existingMatches.count() << "are already in the collection";

    newMatches = newMatches - existingMatches;

    const bool existingTransaction = DataStore::self()->inTransaction();
    if (!existingTransaction) {
        DataStore::self()->beginTransaction();
    }

    QVariantList newMatchesVariant;
    Q_FOREACH (qint64 id, newMatches) {
        newMatchesVariant << id;
        Collection::addPimItem(collection.id(), id);
    }

    akDebug() << "Added" << newMatches.count();

    if (!existingTransaction && !DataStore::self()->commitTransaction()) {
        akDebug() << "Failed to commit transaction";
        return;
    }

    if (!newMatchesVariant.isEmpty()) {
        SelectQueryBuilder<PimItem> qb;
        qb.addValueCondition(PimItem::idFullColumnName(), Query::In, newMatchesVariant);
        if (!qb.exec()) {
            return ;
        }
        const QVector<PimItem> newItems = qb.result();
        DataStore::self()->notificationCollector()->itemsLinked(newItems, collection);
        // Force collector to dispatch the notification now
        DataStore::self()->notificationCollector()->dispatchNotifications();
    }
}
