/*
    Copyright (c) 2017 Daniel Vrátil <dvratil@kde.org>

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

#include "indexer.h"
#include "indexfuture.h"
#include "indextask.h"
#include "abstractindexingplugin.h"
#include "akonadiserver_debug.h"

#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QTimer>
#include <QVector>
#include <QDir>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QDBusConnection>

#include <memory>

namespace Akonadi {
namespace Server {

class IndexerPrivate
{
public:
    void loadIndexingPlugins();
    void initIndexingPlugins();

    QVector<QPluginLoader*> pluginLoaders;
    QVector<AbstractIndexingPlugin *> indexers;
    qint64 lastTaskId = 0;
    QMutex lock;
    QWaitCondition cond;
    QQueue<IndexTask> queue;
    bool shouldStop = false;
    bool enableDebugging = false;
};


}
}

using namespace Akonadi::Server;

IndexFuture Indexer::index(qint64 id, const QString &mimeType, const QByteArray &indexData)
{
    QMutexLocker locker(&d->lock);
    const int taskId = ++d->lastTaskId;
    const IndexTask task{ taskId, id, mimeType, indexData };
    if (d->enableDebugging) {
        QTimer::singleShot(0, this, [=]() {
            Q_EMIT enqueued(taskId, id, mimeType);
        });
    }
    d->queue.enqueue(task);
    locker.unlock();
    d->cond.wakeAll();

    return task.future;
}


void IndexerPrivate::loadIndexingPlugins()
{
    Q_ASSERT(QThread::currentThread() == qApp->thread());

    QStringList loadedPlugins;
    const QString pluginOverride = QString::fromLatin1(qgetenv("AKONADI_OVERRIDE_INDEXINGPLUGIN"));
    if (!pluginOverride.isEmpty()) {
        qCDebug(AKONADISERVER_LOG) << "Overriding the indexing plugins with: " << pluginOverride;
    }

    const QStringList dirs = QCoreApplication::libraryPaths();
    for (const QString &pluginDir : dirs) {
        QDir dir(pluginDir + QLatin1String("/akonadi"));
        const QStringList fileNames = dir.entryList(QDir::Files);
        qCDebug(AKONADISERVER_LOG) << "INDEXER: searching in " << pluginDir + QLatin1String("/akonadi") << ":" << fileNames;
        for (const QString &fileName : fileNames) {
            const QString filePath = pluginDir % QLatin1String("/akonadi/") % fileName;
            std::unique_ptr<QPluginLoader> loader(new QPluginLoader(filePath));
            const QVariantMap metadata = loader->metaData().value(QStringLiteral("MetaData")).toVariant().toMap();
            if (metadata.value(QStringLiteral("X-Akonadi-PluginType")).toString() != QLatin1String("IndexingPlugin")) {
                qCDebug(AKONADISERVER_LOG) << "===>" << fileName << metadata.value(QStringLiteral("X-Akonadi-PluginType")).toString();
                continue;
            }

            const QString libraryName = metadata.value(QStringLiteral("X-Akonadi-Library")).toString();
            if (loadedPlugins.contains(libraryName)) {
                qCDebug(AKONADISERVER_LOG) << "Already loaded one version of this plugin, skipping: " << libraryName;
                continue;
            }

            // When search plugin override is active, ignore all plugins except for the override
            if (!pluginOverride.isEmpty()) {
                if (libraryName != pluginOverride) {
                    qCDebug(AKONADISERVER_LOG) << libraryName << "skipped because of AKONADI_OVERRIDE_INDEXINGPLUGIN";
                    continue;
                }

                // When there's no override, only load plugins enabled by default
            } else if (metadata.value(QStringLiteral("X-Akonadi-LoadByDefault"), true).toBool() == false) {
                continue;
            }

            if (!loader->load()) {
                qCCritical(AKONADISERVER_LOG) << "Failed to load indexing plugin" << libraryName << ":" << loader->errorString();
                continue;
            }

            pluginLoaders << loader.release();
            loadedPlugins << libraryName;
        }
    }
}

void IndexerPrivate::initIndexingPlugins()
{
    for (QPluginLoader *loader : qAsConst(pluginLoaders)) {
        if (!loader->load()) {
            qCCritical(AKONADISERVER_LOG) << "Failed to load search plugin" << loader->fileName() << ":" << loader->errorString();
            continue;
        }

        auto plugin = qobject_cast<AbstractIndexingPlugin *>(loader->instance());
        if (!plugin) {
            qCCritical(AKONADISERVER_LOG) << loader->fileName() << "is not a valid Akonadi indexing plugin";
            continue;
        }

        qCDebug(AKONADISERVER_LOG) << "Indexer: loaded indexing plugin" << loader->fileName();
        indexers << plugin;
    }
}

Indexer *Indexer::sInstance = nullptr;

Indexer::Indexer()
    : AkThread(QStringLiteral("Indexer"), AkThread::ManualStart, QThread::InheritPriority, nullptr)
    , d(new IndexerPrivate)
{
    Q_ASSERT(sInstance == nullptr);
    sInstance = this;

    // We load plugins here so that static initializations happen in the main thread
    d->loadIndexingPlugins();

    QDBusConnection conn = QDBusConnection::sessionBus();
    conn.registerObject(QStringLiteral("/Indexer"), this,
                        QDBusConnection::ExportAllSlots);
     QTimer::singleShot(0, this, &Indexer::indexerLoop);

    // Delay-call init()
    startThread();
}

Indexer::~Indexer()
{
    quitThread();

    sInstance = nullptr;
}

Indexer *Indexer::instance()
{
    Q_ASSERT(sInstance);
    return sInstance;
}

void Indexer::enableDebugging(bool enable)
{
    QMutexLocker locker(&d->lock);
    d->enableDebugging = enable;
}

QList<IndexTask> Indexer::queue()
{
    QMutexLocker locker(&d->lock);
    return d->queue;
}

void Indexer::init()
{
    AkThread::init();

    QMutexLocker locker(&d->lock);
    d->initIndexingPlugins();
}

void Indexer::quit()
{
    qDeleteAll(d->indexers);
    /*
     * FIXME: Unloading plugin messes up some global statics from client libs
     * and causes crash on Akonadi shutdown (below main). Keeping the plugins
     * loaded is not really a big issue as this is only invoked on server shutdown
     * anyway, so we are not leaking any memory.
    Q_FOREACH (QPluginLoader *loader, pluginsLoaders) {
        loader->unload();
        delete loader;
    }
    */

    AkThread::quit();
}

void Indexer::indexerLoop()
{
    QMutexLocker locker(&d->lock);

    Q_FOREVER {
        d->cond.wait(&d->lock);

        if (d->shouldStop) {
            break;
        }

        auto task = d->queue.dequeue();
        locker.unlock();

        for (auto indexer : qAsConst(d->indexers)) {
            if (!indexer->index(task.mimeType, task.entityId, task.data)) {
                qCWarning(AKONADISERVER_LOG) << "Failed to index Item" << task.entityId << "!";
                task.future.setFinished(false);
                break;
            }
        }
        if (!task.future.isFinished()) {
            task.future.setFinished(true);
        }

        locker.relock();
        if (d->enableDebugging) {
            Q_EMIT processed(task.future.taskId(), !task.future.hasError());
        }
    }
}
