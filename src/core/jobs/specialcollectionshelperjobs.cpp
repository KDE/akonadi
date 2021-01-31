/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "specialcollectionshelperjobs_p.h"

#include "servermanager.h"
#include "specialcollectionattribute.h"
#include "specialcollections.h"
#include <QDBusConnection>

#include "agentinstance.h"
#include "agentinstancecreatejob.h"
#include "agentmanager.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "collectionmodifyjob.h"
#include "entitydisplayattribute.h"
#include "resourcesynchronizationjob.h"

#include "akonadicore_debug.h"

#include <KCoreConfigSkeleton>
#include <KLocalizedString>

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QMetaMethod>
#include <QTime>
#include <QTimer>

#define LOCK_WAIT_TIMEOUT_SECONDS 30

using namespace Akonadi;

// convenient methods to get/set the default resource id
static void setDefaultResourceId(KCoreConfigSkeleton *settings, const QString &value)
{
    KConfigSkeletonItem *item = settings->findItem(QStringLiteral("DefaultResourceId"));
    Q_ASSERT(item);
    item->setProperty(value);
}

static QString defaultResourceId(KCoreConfigSkeleton *settings)
{
    const KConfigSkeletonItem *item = settings->findItem(QStringLiteral("DefaultResourceId"));
    Q_ASSERT(item);
    return item->property().toString();
}

static QString dbusServiceName()
{
    QString service = QStringLiteral("org.kde.pim.SpecialCollections");
    if (ServerManager::hasInstanceIdentifier()) {
        return service + ServerManager::instanceIdentifier();
    }
    return service;
}

static QVariant::Type argumentType(const QMetaObject *mo, const QString &method)
{
    QMetaMethod m;
    for (int i = 0; i < mo->methodCount(); ++i) {
        const QString signature = QString::fromLatin1(mo->method(i).methodSignature());
        if (signature.startsWith(method)) {
            m = mo->method(i);
        }
    }

    if (m.methodSignature().isEmpty()) {
        return QVariant::Invalid;
    }

    const QList<QByteArray> argTypes = m.parameterTypes();
    if (argTypes.count() != 1) {
        return QVariant::Invalid;
    }

    return QVariant::nameToType(argTypes.first().constData());
}

// ===================== ResourceScanJob ============================

/**
  @internal
*/
class Q_DECL_HIDDEN Akonadi::ResourceScanJob::Private
{
public:
    Private(KCoreConfigSkeleton *settings, ResourceScanJob *qq);

    void fetchResult(KJob *job); // slot

    ResourceScanJob *const q;

    // Input:
    QString mResourceId;
    KCoreConfigSkeleton *mSettings = nullptr;

    // Output:
    Collection mRootCollection;
    Collection::List mSpecialCollections;
};

ResourceScanJob::Private::Private(KCoreConfigSkeleton *settings, ResourceScanJob *qq)
    : q(qq)
    , mSettings(settings)
{
}

void ResourceScanJob::Private::fetchResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorText();
        return;
    }

    auto *fetchJob = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(fetchJob);

    Q_ASSERT(!mRootCollection.isValid());
    Q_ASSERT(mSpecialCollections.isEmpty());
    const Akonadi::Collection::List lstCols = fetchJob->collections();
    for (const Collection &collection : lstCols) {
        if (collection.parentCollection() == Collection::root()) {
            if (mRootCollection.isValid()) {
                qCWarning(AKONADICORE_LOG) << "Resource has more than one root collection. I don't know what to do.";
            } else {
                mRootCollection = collection;
            }
        }

        if (collection.hasAttribute<SpecialCollectionAttribute>()) {
            mSpecialCollections.append(collection);
        }
    }

    qCDebug(AKONADICORE_LOG) << "Fetched root collection" << mRootCollection.id() << "and" << mSpecialCollections.count() << "local folders"
                             << "(total" << fetchJob->collections().count() << "collections).";

    if (!mRootCollection.isValid()) {
        q->setError(Unknown);
        q->setErrorText(i18n("Could not fetch root collection of resource %1.", mResourceId));
        q->emitResult();
        return;
    }

    // We are done!
    q->emitResult();
}

ResourceScanJob::ResourceScanJob(const QString &resourceId, KCoreConfigSkeleton *settings, QObject *parent)
    : Job(parent)
    , d(new Private(settings, this))
{
    setResourceId(resourceId);
}

ResourceScanJob::~ResourceScanJob()
{
    delete d;
}

QString ResourceScanJob::resourceId() const
{
    return d->mResourceId;
}

void ResourceScanJob::setResourceId(const QString &resourceId)
{
    d->mResourceId = resourceId;
}

Akonadi::Collection ResourceScanJob::rootResourceCollection() const
{
    return d->mRootCollection;
}

Akonadi::Collection::List ResourceScanJob::specialCollections() const
{
    return d->mSpecialCollections;
}

void ResourceScanJob::doStart()
{
    if (d->mResourceId.isEmpty()) {
        if (!qobject_cast<DefaultResourceJob *>(this)) {
            qCCritical(AKONADICORE_LOG) << "No resource ID given.";
            setError(Job::Unknown);
            setErrorText(i18n("No resource ID given."));
        }
        emitResult();
        return;
    }

    CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    fetchJob->fetchScope().setResource(d->mResourceId);
    fetchJob->fetchScope().setIncludeStatistics(true);
    fetchJob->fetchScope().setListFilter(CollectionFetchScope::Display);
    connect(fetchJob, &CollectionFetchJob::result, this, [this](KJob *job) {
        d->fetchResult(job);
    });
}

// ===================== DefaultResourceJob ============================

/**
  @internal
*/
class Akonadi::DefaultResourceJobPrivate
{
public:
    DefaultResourceJobPrivate(KCoreConfigSkeleton *settings, DefaultResourceJob *qq);

    void tryFetchResource();
    void resourceCreateResult(KJob *job); // slot
    void resourceSyncResult(KJob *job); // slot
    void collectionFetchResult(KJob *job); // slot
    void collectionModifyResult(KJob *job); // slot

    DefaultResourceJob *const q;
    KCoreConfigSkeleton *mSettings = nullptr;
    QVariantMap mDefaultResourceOptions;
    QList<QByteArray> mKnownTypes;
    QMap<QByteArray, QString> mNameForTypeMap;
    QMap<QByteArray, QString> mIconForTypeMap;
    QString mDefaultResourceType;
    int mPendingModifyJobs = 0;
    bool mResourceWasPreexisting = true;
};

DefaultResourceJobPrivate::DefaultResourceJobPrivate(KCoreConfigSkeleton *settings, DefaultResourceJob *qq)
    : q(qq)
    , mSettings(settings)
    , mPendingModifyJobs(0)
    , mResourceWasPreexisting(true /* for safety, so as not to accidentally delete data */)
{
}

void DefaultResourceJobPrivate::tryFetchResource()
{
    // Get the resourceId from config. Another instance might have changed it in the meantime.
    mSettings->load();

    const QString resourceId = defaultResourceId(mSettings);

    qCDebug(AKONADICORE_LOG) << "Read defaultResourceId" << resourceId << "from config.";

    const AgentInstance resource = AgentManager::self()->instance(resourceId);
    if (resource.isValid()) {
        // The resource exists; scan it.
        mResourceWasPreexisting = true;
        qCDebug(AKONADICORE_LOG) << "Found resource" << resourceId;
        q->setResourceId(resourceId);

        CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, q);
        fetchJob->fetchScope().setResource(resourceId);
        fetchJob->fetchScope().setIncludeStatistics(true);
        q->connect(fetchJob, &CollectionFetchJob::result, q, [this](KJob *job) {
            collectionFetchResult(job);
        });
    } else {
        // Try harder: maybe the default resource has been removed and another one added
        //             without updating the config file, in this case search for a resource
        //             of the same type and the default name
        const AgentInstance::List resources = AgentManager::self()->instances();
        for (const AgentInstance &resource : resources) {
            if (resource.type().identifier() == mDefaultResourceType) {
                if (resource.name() == mDefaultResourceOptions.value(QStringLiteral("Name")).toString()) {
                    // found a matching one...
                    setDefaultResourceId(mSettings, resource.identifier());
                    mSettings->save();
                    mResourceWasPreexisting = true;
                    qCDebug(AKONADICORE_LOG) << "Found resource" << resource.identifier();
                    q->setResourceId(resource.identifier());
                    q->ResourceScanJob::doStart();
                    return;
                }
            }
        }

        // Create the resource.
        mResourceWasPreexisting = false;
        qCDebug(AKONADICORE_LOG) << "Creating maildir resource.";
        const AgentType type = AgentManager::self()->type(mDefaultResourceType);
        auto *job = new AgentInstanceCreateJob(type, q);
        QObject::connect(job, &AgentInstanceCreateJob::result, q, [this](KJob *job) {
            resourceCreateResult(job);
        });
        job->start(); // non-Akonadi::Job
    }
}

void DefaultResourceJobPrivate::resourceCreateResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorText();
        // fail( i18n( "Failed to create the default resource (%1).", job->errorString() ) );
        q->setError(job->error());
        q->setErrorText(job->errorText());
        q->emitResult();
        return;
    }

    AgentInstance agent;

    // Get the resource instance.
    {
        auto *createJob = qobject_cast<AgentInstanceCreateJob *>(job);
        Q_ASSERT(createJob);
        agent = createJob->instance();
        setDefaultResourceId(mSettings, agent.identifier());
        qCDebug(AKONADICORE_LOG) << "Created maildir resource with id" << defaultResourceId(mSettings);
    }

    const QString defaultId = defaultResourceId(mSettings);

    // Configure the resource.
    {
        agent.setName(mDefaultResourceOptions.value(QStringLiteral("Name")).toString());

        const auto service = ServerManager::agentServiceName(ServerManager::Resource, defaultId);
        QDBusInterface conf(service, QStringLiteral("/Settings"), QString());

        if (!conf.isValid()) {
            q->setError(-1);
            q->setErrorText(i18n("Invalid resource identifier '%1'", defaultId));
            q->emitResult();
            return;
        }

        QMap<QString, QVariant>::const_iterator it = mDefaultResourceOptions.cbegin();
        const QMap<QString, QVariant>::const_iterator itEnd = mDefaultResourceOptions.cend();
        for (; it != itEnd; ++it) {
            if (it.key() == QLatin1String("Name")) {
                continue;
            }

            const QString methodName = QStringLiteral("set%1").arg(it.key());
            const QVariant::Type argType = argumentType(conf.metaObject(), methodName);
            if (argType == QVariant::Invalid) {
                q->setError(Job::Unknown);
                q->setErrorText(i18n("Failed to configure default resource via D-Bus."));
                q->emitResult();
                return;
            }

            QDBusReply<void> reply = conf.call(methodName, it.value());
            if (!reply.isValid()) {
                q->setError(Job::Unknown);
                q->setErrorText(i18n("Failed to configure default resource via D-Bus."));
                q->emitResult();
                return;
            }
        }

        conf.call(QStringLiteral("save"));

        agent.reconfigure();
    }

    // Sync the resource.
    {
        auto *syncJob = new ResourceSynchronizationJob(agent, q);
        QObject::connect(syncJob, &ResourceSynchronizationJob::result, q, [this](KJob *job) {
            resourceSyncResult(job);
        });
        syncJob->start(); // non-Akonadi
    }
}

void DefaultResourceJobPrivate::resourceSyncResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorText();
        // fail( i18n( "ResourceSynchronizationJob failed (%1).", job->errorString() ) );
        return;
    }

    // Fetch the collections of the resource.
    qCDebug(AKONADICORE_LOG) << "Fetching maildir collections.";
    CollectionFetchJob *fetchJob = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, q);
    fetchJob->fetchScope().setResource(defaultResourceId(mSettings));
    QObject::connect(fetchJob, &CollectionFetchJob::result, q, [this](KJob *job) {
        collectionFetchResult(job);
    });
}

void DefaultResourceJobPrivate::collectionFetchResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorText();
        // fail( i18n( "Failed to fetch the root maildir collection (%1).", job->errorString() ) );
        return;
    }

    auto *fetchJob = qobject_cast<CollectionFetchJob *>(job);
    Q_ASSERT(fetchJob);

    const Collection::List collections = fetchJob->collections();
    qCDebug(AKONADICORE_LOG) << "Fetched" << collections.count() << "collections.";

    // Find the root maildir collection.
    Collection::List toRecover;
    Collection resourceCollection;
    for (const Collection &collection : collections) {
        if (collection.parentCollection() == Collection::root()) {
            resourceCollection = collection;
            toRecover.append(collection);
            break;
        }
    }

    if (!resourceCollection.isValid()) {
        q->setError(Job::Unknown);
        q->setErrorText(i18n("Failed to fetch the resource collection."));
        q->emitResult();
        return;
    }

    // Find all children of the resource collection.
    for (const Collection &collection : qAsConst(collections)) {
        if (collection.parentCollection() == resourceCollection) {
            toRecover.append(collection);
        }
    }

    QHash<QString, QByteArray> typeForName;
    for (const QByteArray &type : qAsConst(mKnownTypes)) {
        const QString displayName = mNameForTypeMap.value(type);
        typeForName[displayName] = type;
    }

    // These collections have been created by the maildir resource, when it
    // found the folders on disk. So give them the necessary attributes now.
    Q_ASSERT(mPendingModifyJobs == 0);
    for (Collection collection : qAsConst(toRecover)) {
        if (collection.hasAttribute<SpecialCollectionAttribute>()) {
            continue;
        }

        // Find the type for the collection.
        const QString name = collection.displayName();
        const QByteArray type = typeForName.value(name);

        if (!type.isEmpty()) {
            qCDebug(AKONADICORE_LOG) << "Recovering collection" << name;
            setCollectionAttributes(collection, type, mNameForTypeMap, mIconForTypeMap);

            auto *modifyJob = new CollectionModifyJob(collection, q);
            QObject::connect(modifyJob, &CollectionModifyJob::result, q, [this](KJob *job) {
                collectionModifyResult(job);
            });
            mPendingModifyJobs++;
        } else {
            qCDebug(AKONADICORE_LOG) << "Searching for names: " << typeForName.keys();
            qCDebug(AKONADICORE_LOG) << "Unknown collection name" << name << "-- not recovering.";
        }
    }

    if (mPendingModifyJobs == 0) {
        // Scan the resource.
        q->setResourceId(defaultResourceId(mSettings));
        q->ResourceScanJob::doStart();
    }
}

void DefaultResourceJobPrivate::collectionModifyResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorText();
        // fail( i18n( "Failed to modify the root maildir collection (%1).", job->errorString() ) );
        return;
    }

    Q_ASSERT(mPendingModifyJobs > 0);
    mPendingModifyJobs--;
    qCDebug(AKONADICORE_LOG) << "pendingModifyJobs now" << mPendingModifyJobs;
    if (mPendingModifyJobs == 0) {
        // Write the updated config.
        qCDebug(AKONADICORE_LOG) << "Writing defaultResourceId" << defaultResourceId(mSettings) << "to config.";
        mSettings->save();

        // Scan the resource.
        q->setResourceId(defaultResourceId(mSettings));
        q->ResourceScanJob::doStart();
    }
}

DefaultResourceJob::DefaultResourceJob(KCoreConfigSkeleton *settings, QObject *parent)
    : ResourceScanJob(QString(), settings, parent)
    , d(new DefaultResourceJobPrivate(settings, this))
{
}

DefaultResourceJob::~DefaultResourceJob()
{
    delete d;
}

void DefaultResourceJob::setDefaultResourceType(const QString &type)
{
    d->mDefaultResourceType = type;
}

void DefaultResourceJob::setDefaultResourceOptions(const QVariantMap &options)
{
    d->mDefaultResourceOptions = options;
}

void DefaultResourceJob::setTypes(const QList<QByteArray> &types)
{
    d->mKnownTypes = types;
}

void DefaultResourceJob::setNameForTypeMap(const QMap<QByteArray, QString> &map)
{
    d->mNameForTypeMap = map;
}

void DefaultResourceJob::setIconForTypeMap(const QMap<QByteArray, QString> &map)
{
    d->mIconForTypeMap = map;
}

void DefaultResourceJob::doStart()
{
    d->tryFetchResource();
}

void DefaultResourceJob::slotResult(KJob *job)
{
    if (job->error()) {
        qCWarning(AKONADICORE_LOG) << job->errorText();
        // Do some cleanup.
        if (!d->mResourceWasPreexisting) {
            // We only removed the resource instance if we have created it.
            // Otherwise we might lose the user's data.
            const AgentInstance resource = AgentManager::self()->instance(defaultResourceId(d->mSettings));
            qCDebug(AKONADICORE_LOG) << "Removing resource" << resource.identifier();
            AgentManager::self()->removeInstance(resource);
        }
    }

    Job::slotResult(job);
}

// ===================== GetLockJob ============================

class Q_DECL_HIDDEN Akonadi::GetLockJob::Private
{
public:
    explicit Private(GetLockJob *qq);

    void doStart(); // slot
    void timeout(); // slot

    GetLockJob *const q;
    QTimer *mSafetyTimer = nullptr;
};

GetLockJob::Private::Private(GetLockJob *qq)
    : q(qq)
    , mSafetyTimer(nullptr)
{
}

void GetLockJob::Private::doStart()
{
    // Just doing registerService() and checking its return value is not sufficient,
    // since we may *already* own the name, and then registerService() returns true.

    QDBusConnection bus = QDBusConnection::sessionBus();
    const bool alreadyLocked = bus.interface()->isServiceRegistered(dbusServiceName());
    const bool gotIt = bus.registerService(dbusServiceName());

    if (gotIt && !alreadyLocked) {
        // qCDebug(AKONADICORE_LOG) << "Got lock immediately.";
        q->emitResult();
    } else {
        auto *watcher = new QDBusServiceWatcher(dbusServiceName(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration, q);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, q, [this]() {
            if (QDBusConnection::sessionBus().registerService(dbusServiceName())) {
                mSafetyTimer->stop();
                q->emitResult();
            }
        });

        mSafetyTimer = new QTimer(q);
        mSafetyTimer->setSingleShot(true);
        mSafetyTimer->setInterval(LOCK_WAIT_TIMEOUT_SECONDS * 1000);
        mSafetyTimer->start();
        connect(mSafetyTimer, &QTimer::timeout, q, [this]() {
            timeout();
        });
    }
}

void GetLockJob::Private::timeout()
{
    qCWarning(AKONADICORE_LOG) << "Timeout trying to get lock. Check who has acquired the name" << dbusServiceName() << "on DBus, using qdbus or qdbusviewer.";
    q->setError(Job::Unknown);
    q->setErrorText(i18n("Timeout trying to get lock."));
    q->emitResult();
}

GetLockJob::GetLockJob(QObject *parent)
    : KJob(parent)
    , d(new Private(this))
{
}

GetLockJob::~GetLockJob()
{
    delete d;
}

void GetLockJob::start()
{
    QTimer::singleShot(0, this, [this]() {
        d->doStart();
    });
}

void Akonadi::setCollectionAttributes(Akonadi::Collection &collection,
                                      const QByteArray &type,
                                      const QMap<QByteArray, QString> &nameForType,
                                      const QMap<QByteArray, QString> &iconForType)
{
    {
        auto *attr = new EntityDisplayAttribute;
        attr->setIconName(iconForType.value(type));
        attr->setDisplayName(nameForType.value(type));
        collection.addAttribute(attr);
    }

    {
        SpecialCollectionAttribute *attr = new SpecialCollectionAttribute;
        attr->setCollectionType(type);
        collection.addAttribute(attr);
    }
}

bool Akonadi::releaseLock()
{
    return QDBusConnection::sessionBus().unregisterService(dbusServiceName());
}

#include "moc_specialcollectionshelperjobs_p.cpp"
