/*
    SPDX-FileCopyrightText: 2006 Till Adam <adam@kde.org>
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2007 Bruno Virlet <bruno.virlet@gmail.com>
    SPDX-FileCopyrightText: 2008 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentbase.h"
#include "agentbase_p.h"

#include "agentconfigurationdialog.h"
#include "agentmanager.h"
#include "akonadifull-version.h"
#include "changerecorder.h"
#include "controladaptor.h"
#include "itemfetchjob.h"
#include "monitor_p.h"
#include "private/standarddirs_p.h"
#include "servermanager_p.h"
#include "session.h"
#include "session_p.h"
#include "statusadaptor.h"

#include "akonadiagentbase_debug.h"

#include <KLocalizedString>

#include <KAboutData>

#include <QCommandLineParser>
#include <QNetworkInformation>
#include <QPointer>
#include <QSettings>
#include <QTimer>

#include <QStandardPaths>
#include <signal.h>
#include <stdlib.h>
#if defined __GLIBC__
#include <malloc.h> // for dumping memory information
#endif

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include <chrono>
#include <thread>

using namespace Akonadi;
using namespace std::chrono_literals;
static AgentBase *sAgentBase = nullptr;

AgentBase::Observer::Observer()
{
}

AgentBase::Observer::~Observer()
{
}

void AgentBase::Observer::itemAdded(const Item &item, const Collection &collection)
{
    Q_UNUSED(item)
    Q_UNUSED(collection)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::itemChanged(const Item &item, const QSet<QByteArray> &partIdentifiers)
{
    Q_UNUSED(item)
    Q_UNUSED(partIdentifiers)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::itemRemoved(const Item &item)
{
    Q_UNUSED(item)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(collection)
    Q_UNUSED(parent)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::collectionChanged(const Collection &collection)
{
    Q_UNUSED(collection)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::collectionRemoved(const Collection &collection)
{
    Q_UNUSED(collection)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    Q_UNUSED(item)
    Q_UNUSED(source)
    Q_UNUSED(dest)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(item)
    Q_UNUSED(collection)
    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemLinked, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemLinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(item)
    Q_UNUSED(collection)
    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemUnlinked, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemUnlinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    Q_UNUSED(collection)
    Q_UNUSED(source)
    Q_UNUSED(dest)
    if (sAgentBase) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes)
{
    Q_UNUSED(changedAttributes)
    collectionChanged(collection);
}

void AgentBase::ObserverV3::itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags)
{
    Q_UNUSED(items)
    Q_UNUSED(addedFlags)
    Q_UNUSED(removedFlags)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsFlagsChanged, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemsFlagsChanged);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsMoved(const Akonadi::Item::List &items, const Collection &sourceCollection, const Collection &destinationCollection)
{
    Q_UNUSED(items)
    Q_UNUSED(sourceCollection)
    Q_UNUSED(destinationCollection)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsMoved, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemsMoved);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsRemoved(const Akonadi::Item::List &items)
{
    Q_UNUSED(items)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsRemoved, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemsRemoved);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsLinked(const Akonadi::Item::List &items, const Collection &collection)
{
    Q_UNUSED(items)
    Q_UNUSED(collection)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsLinked, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemsLinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsUnlinked(const Akonadi::Item::List &items, const Collection &collection)
{
    Q_UNUSED(items)
    Q_UNUSED(collection)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsUnlinked, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemsUnlinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::tagAdded(const Tag &tag)
{
    Q_UNUSED(tag)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::tagAdded, sAgentBase->d_ptr.get(), &AgentBasePrivate::tagAdded);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::tagChanged(const Tag &tag)
{
    Q_UNUSED(tag)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::tagChanged, sAgentBase->d_ptr.get(), &AgentBasePrivate::tagChanged);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::tagRemoved(const Tag &tag)
{
    Q_UNUSED(tag)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::tagRemoved, sAgentBase->d_ptr.get(), &AgentBasePrivate::tagRemoved);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::itemsTagsChanged(const Item::List &items, const QSet<Tag> &addedTags, const QSet<Tag> &removedTags)
{
    Q_UNUSED(items)
    Q_UNUSED(addedTags)
    Q_UNUSED(removedTags)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsTagsChanged, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemsTagsChanged);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::relationAdded(const Akonadi::Relation &relation)
{
    Q_UNUSED(relation)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::relationAdded, sAgentBase->d_ptr.get(), &AgentBasePrivate::relationAdded);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::relationRemoved(const Akonadi::Relation &relation)
{
    Q_UNUSED(relation)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::relationRemoved, sAgentBase->d_ptr.get(), &AgentBasePrivate::relationRemoved);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::itemsRelationsChanged(const Akonadi::Item::List &items,
                                                  const Akonadi::Relation::List &addedRelations,
                                                  const Akonadi::Relation::List &removedRelations)
{
    Q_UNUSED(items)
    Q_UNUSED(addedRelations)
    Q_UNUSED(removedRelations)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        disconnect(sAgentBase->changeRecorder(), &Monitor::itemsRelationsChanged, sAgentBase->d_ptr.get(), &AgentBasePrivate::itemsRelationsChanged);
        sAgentBase->d_ptr->changeProcessed();
    }
}

/// @cond PRIVATE

AgentBasePrivate::AgentBasePrivate(AgentBase *parent)
    : q_ptr(parent)
    , mStatusCode(AgentBase::Idle)
    , mProgress(0)
    , mNeedsNetwork(false)
    , mOnline(false)
    , mDesiredOnlineState(false)
    , mPendingQuit(false)
    , mSettings(nullptr)
    , mChangeRecorder(nullptr)
    , mTracer(nullptr)
    , mObserver(nullptr)
    , mPowerInterface(nullptr)
    , mTemporaryOfflineTimer(nullptr)
    , mEventLoopLocker(nullptr)
{
    Internal::setClientType(Internal::Agent);
}

AgentBasePrivate::~AgentBasePrivate()
{
    mChangeRecorder->setConfig(nullptr);
    delete mSettings;
}

void AgentBasePrivate::init()
{
    Q_Q(AgentBase);
    /**
     * Create a default session for this process.
     */
    SessionPrivate::createDefaultSession(mId.toLatin1());

    mTracer =
        new org::freedesktop::Akonadi::Tracer(ServerManager::serviceName(ServerManager::Server), QStringLiteral("/tracing"), QDBusConnection::sessionBus(), q);

    new Akonadi__ControlAdaptor(q);
    new Akonadi__StatusAdaptor(q);
    if (!QDBusConnection::sessionBus().registerObject(QStringLiteral("/"), q, QDBusConnection::ExportAdaptors)) {
        Q_EMIT q->error(i18n("Unable to register object at dbus: %1", QDBusConnection::sessionBus().lastError().message()));
    }

    mSettings = new QSettings(ServerManager::agentConfigFilePath(mId), QSettings::IniFormat);

    mChangeRecorder = new ChangeRecorder(q);
    mChangeRecorder->setObjectName(QLatin1StringView("AgentBaseChangeRecorder"));
    mChangeRecorder->ignoreSession(Session::defaultSession());
    mChangeRecorder->itemFetchScope().setCacheOnly(true);
    mChangeRecorder->setConfig(mSettings);

    mDesiredOnlineState = mSettings->value(QStringLiteral("Agent/DesiredOnlineState"), true).toBool();
    mOnline = mDesiredOnlineState;

    // reinitialize the status message now that online state is available
    mStatusMessage = defaultReadyMessage();

    mName = mSettings->value(QStringLiteral("Agent/Name")).toString();
    if (mName.isEmpty()) {
        mName = mSettings->value(QStringLiteral("Resource/Name")).toString();
        if (!mName.isEmpty()) {
            mSettings->remove(QStringLiteral("Resource/Name"));
            mSettings->setValue(QStringLiteral("Agent/Name"), mName);
        }
    }

    connect(mChangeRecorder, &Monitor::itemAdded, this, &AgentBasePrivate::itemAdded);
    connect(mChangeRecorder, &Monitor::itemChanged, this, &AgentBasePrivate::itemChanged);
    connect(mChangeRecorder, &Monitor::collectionAdded, this, &AgentBasePrivate::collectionAdded);
    connect(mChangeRecorder,
            qOverload<const Collection &>(&ChangeRecorder::collectionChanged),
            this,
            qOverload<const Collection &>(&AgentBasePrivate::collectionChanged));
    connect(mChangeRecorder,
            qOverload<const Collection &, const QSet<QByteArray> &>(&ChangeRecorder::collectionChanged),
            this,
            qOverload<const Collection &, const QSet<QByteArray> &>(&AgentBasePrivate::collectionChanged));
    connect(mChangeRecorder, &Monitor::collectionMoved, this, &AgentBasePrivate::collectionMoved);
    connect(mChangeRecorder, &Monitor::collectionRemoved, this, &AgentBasePrivate::collectionRemoved);
    connect(mChangeRecorder, &Monitor::collectionSubscribed, this, &AgentBasePrivate::collectionSubscribed);
    connect(mChangeRecorder, &Monitor::collectionUnsubscribed, this, &AgentBasePrivate::collectionUnsubscribed);

    connect(q, qOverload<int, const QString &>(&AgentBase::status), this, &AgentBasePrivate::slotStatus);
    connect(q, &AgentBase::percent, this, &AgentBasePrivate::slotPercent);
    connect(q, &AgentBase::warning, this, &AgentBasePrivate::slotWarning);
    connect(q, &AgentBase::error, this, &AgentBasePrivate::slotError);

    mPowerInterface = new QDBusInterface(QStringLiteral("org.kde.Solid.PowerManagement"),
                                         QStringLiteral("/org/kde/Solid/PowerManagement/Actions/SuspendSession"),
                                         QStringLiteral("org.kde.Solid.PowerManagement.Actions.SuspendSession"),
                                         QDBusConnection::sessionBus(),
                                         this);
    if (mPowerInterface->isValid()) {
        connect(mPowerInterface, SIGNAL(resumingFromSuspend()), this, SLOT(slotResumedFromSuspend())); // clazy:exclude=old-style-connect
    } else {
        delete mPowerInterface;
        mPowerInterface = nullptr;
    }

    // Use reference counting to allow agents to finish internal jobs when the
    // agent is stopped.
    mEventLoopLocker = new QEventLoopLocker();

    mResourceTypeName = AgentManager::self()->instance(mId).type().name();
    setProgramName();

    QTimer::singleShot(0s, q, [this] {
        delayedInit();
    });
}

void AgentBasePrivate::delayedInit()
{
    Q_Q(AgentBase);

    const QString serviceId = ServerManager::agentServiceName(ServerManager::Agent, mId);
    if (!QDBusConnection::sessionBus().registerService(serviceId)) {
        qCCritical(AKONADIAGENTBASE_LOG) << "Unable to register service" << serviceId << "at dbus:" << QDBusConnection::sessionBus().lastError().message();
    }
    q->setOnlineInternal(mDesiredOnlineState);

    QDBusConnection::sessionBus().registerObject(QStringLiteral("/Debug"), this, QDBusConnection::ExportScriptableSlots);
}

void AgentBasePrivate::setProgramName()
{
    // ugly, really ugly, if you find another solution, change it and blame me for this code (Andras)
    QString programName = mResourceTypeName;
    if (!mName.isEmpty()) {
        programName = i18nc("Name and type of Akonadi resource", "%1 of type %2", mName, mResourceTypeName);
    }

    QGuiApplication::setApplicationDisplayName(programName);
}

void AgentBasePrivate::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (mObserver) {
        mObserver->itemAdded(item, collection);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    if (mObserver) {
        mObserver->itemChanged(item, partIdentifiers);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    auto observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (mObserver) {
        // inter-resource moves, requires we know which resources the source and destination are in though
        if (!source.resource().isEmpty() && !dest.resource().isEmpty()) {
            if (source.resource() != dest.resource()) {
                if (source.resource() == q_ptr->identifier()) { // moved away from us
                    Akonadi::Item i(item);
                    i.setParentCollection(source);
                    mObserver->itemRemoved(i);
                } else if (dest.resource() == q_ptr->identifier()) { // moved to us
                    mObserver->itemAdded(item, dest);
                } else if (observer2) {
                    observer2->itemMoved(item, source, dest);
                } else {
                    // not for us, not sure if we should get here at all
                    changeProcessed();
                }
                return;
            }
        }
        // intra-resource move
        if (observer2) {
            observer2->itemMoved(item, source, dest);
        } else {
            // ### we cannot just call itemRemoved here as this will already trigger changeProcessed()
            // so, just itemAdded() is good enough as no resource can have implemented intra-resource moves anyway
            // without using ObserverV2
            mObserver->itemAdded(item, dest);
            // mObserver->itemRemoved( item );
        }
    }
}

void AgentBasePrivate::itemRemoved(const Akonadi::Item &item)
{
    if (mObserver) {
        mObserver->itemRemoved(item);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    auto observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (observer2) {
        observer2->itemLinked(item, collection);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    auto observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (observer2) {
        observer2->itemUnlinked(item, collection);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags)
{
    auto observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsFlagsChanged(items, addedFlags, removedFlags);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &source, const Akonadi::Collection &destination)
{
    auto observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsMoved(items, source, destination);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::itemsRemoved(const Akonadi::Item::List &items)
{
    auto observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsRemoved(items);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::itemsLinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection)
{
    if (!mObserver) {
        changeProcessed();
        return;
    }

    auto observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsLinked(items, collection);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection)
{
    if (!mObserver) {
        changeProcessed();
        return;
    }

    auto observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsUnlinked(items, collection);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::tagAdded(const Akonadi::Tag &tag)
{
    auto observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->tagAdded(tag);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::tagChanged(const Akonadi::Tag &tag)
{
    auto observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->tagChanged(tag);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::tagRemoved(const Akonadi::Tag &tag)
{
    auto observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->tagRemoved(tag);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags)
{
    auto observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->itemsTagsChanged(items, addedTags, removedTags);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::relationAdded(const Akonadi::Relation &relation)
{
    auto observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->relationAdded(relation);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::relationRemoved(const Akonadi::Relation &relation)
{
    auto observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->relationRemoved(relation);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemsRelationsChanged(const Akonadi::Item::List &items,
                                             const Akonadi::Relation::List &addedRelations,
                                             const Akonadi::Relation::List &removedRelations)
{
    auto observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->itemsRelationsChanged(items, addedRelations, removedRelations);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    if (mObserver) {
        mObserver->collectionAdded(collection, parent);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::collectionChanged(const Akonadi::Collection &collection)
{
    auto observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (mObserver && observer2 == nullptr) { // For ObserverV2 we use the variant with the part identifiers
        mObserver->collectionChanged(collection);
    } else if (!mObserver) {
        changeProcessed();
    }
}

void AgentBasePrivate::collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes)
{
    auto observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (observer2) {
        observer2->collectionChanged(collection, changedAttributes);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    auto observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (observer2) {
        observer2->collectionMoved(collection, source, dest);
    } else if (mObserver) {
        // ### we cannot just call collectionRemoved here as this will already trigger changeProcessed()
        // so, just collectionAdded() is good enough as no resource can have implemented intra-resource moves anyway
        // without using ObserverV2
        mObserver->collectionAdded(collection, dest);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::collectionRemoved(const Akonadi::Collection &collection)
{
    if (mObserver) {
        mObserver->collectionRemoved(collection);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::collectionSubscribed(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(collection)
    Q_UNUSED(parent)
    changeProcessed();
}

void AgentBasePrivate::collectionUnsubscribed(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection)
    changeProcessed();
}

void AgentBasePrivate::changeProcessed()
{
    mChangeRecorder->changeProcessed();
    QTimer::singleShot(0s, mChangeRecorder, &ChangeRecorder::replayNext);
}

void AgentBasePrivate::slotStatus(int status, const QString &message)
{
    mStatusMessage = message;
    mStatusCode = 0;

    switch (status) {
    case AgentBase::Idle:
        if (mStatusMessage.isEmpty()) {
            mStatusMessage = defaultReadyMessage();
        }

        mStatusCode = 0;
        break;
    case AgentBase::Running:
        if (mStatusMessage.isEmpty()) {
            mStatusMessage = defaultSyncingMessage();
        }

        mStatusCode = 1;
        break;
    case AgentBase::Broken:
        if (mStatusMessage.isEmpty()) {
            mStatusMessage = defaultErrorMessage();
        }

        mStatusCode = 2;
        break;

    case AgentBase::NotConfigured:
        if (mStatusMessage.isEmpty()) {
            mStatusMessage = defaultUnconfiguredMessage();
        }

        mStatusCode = 3;
        break;

    default:
        Q_ASSERT(!"Unknown status passed");
        break;
    }
}

void AgentBasePrivate::slotPercent(int progress)
{
    mProgress = progress;
}

void AgentBasePrivate::slotWarning(const QString &message)
{
    mTracer->warning(QStringLiteral("AgentBase(%1)").arg(mId), message);
}

void AgentBasePrivate::slotError(const QString &message)
{
    mTracer->error(QStringLiteral("AgentBase(%1)").arg(mId), message);
}

void AgentBasePrivate::slotNetworkStatusChange(bool isOnline)
{
    Q_UNUSED(isOnline)
    Q_Q(AgentBase);
    q->setOnlineInternal(mDesiredOnlineState);
}

void AgentBasePrivate::slotResumedFromSuspend()
{
    if (mNeedsNetwork) {
        slotNetworkStatusChange(QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Online);
    }
}

void AgentBasePrivate::slotTemporaryOfflineTimeout()
{
    Q_Q(AgentBase);
    q->setOnlineInternal(true);
}

QString AgentBasePrivate::dumpNotificationListToString() const
{
    return mChangeRecorder->dumpNotificationListToString();
}

void AgentBasePrivate::dumpMemoryInfo() const
{
    // Send it to stdout, so we can debug user problems.
    // since you have to explicitly call this
    // it won't flood users with release builds.
    QTextStream stream(stdout);
    stream << dumpMemoryInfoToString();
}

QString AgentBasePrivate::dumpMemoryInfoToString() const
{
    // man mallinfo for more info
    QString str;
#if defined __GLIBC__
    struct mallinfo mi;
    mi = mallinfo();
    QTextStream stream(&str);
    stream << "Total non-mmapped bytes (arena):      " << mi.arena << '\n'
           << "# of free chunks (ordblks):           " << mi.ordblks << '\n'
           << "# of free fastbin blocks (smblks>:    " << mi.smblks << '\n'
           << "# of mapped regions (hblks):          " << mi.hblks << '\n'
           << "Bytes in mapped regions (hblkhd):     " << mi.hblkhd << '\n'
           << "Max. total allocated space (usmblks): " << mi.usmblks << '\n'
           << "Free bytes held in fastbins (fsmblks):" << mi.fsmblks << '\n'
           << "Total allocated space (uordblks):     " << mi.uordblks << '\n'
           << "Total free space (fordblks):          " << mi.fordblks << '\n'
           << "Topmost releasable block (keepcost):  " << mi.keepcost << '\n';
#else
    str = QLatin1String("mallinfo() not supported");
#endif
    return str;
}

AgentBase::AgentBase(const QString &id)
    : d_ptr(new AgentBasePrivate(this))
{
    sAgentBase = this;
    d_ptr->mId = id;
    d_ptr->init();
}

AgentBase::AgentBase(AgentBasePrivate *d, const QString &id)
    : d_ptr(d)
{
    sAgentBase = this;
    d_ptr->mId = id;
    d_ptr->init();
}

AgentBase::~AgentBase() = default;

void AgentBase::debugAgent(int argc, char **argv)
{
    Q_UNUSED(argc)
#ifdef Q_OS_WIN
    if (qEnvironmentVariableIsSet("AKONADI_DEBUG_WAIT")) {
        if (QByteArray(argv[0]).endsWith(QByteArray(qgetenv("AKONADI_DEBUG_WAIT") + QByteArrayLiteral(".exe")))) {
            while (!IsDebuggerPresent()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            DebugBreak();
        }
    }
#else
    Q_UNUSED(argv)
#endif
}

QString AgentBase::parseArguments(int argc, char **argv)
{
    Q_UNUSED(argc)

    QCommandLineOption identifierOption(QStringLiteral("identifier"), i18n("Agent identifier"), QStringLiteral("argument"));
    QCommandLineParser parser;
    parser.addOption(identifierOption);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(*qApp);
    parser.setApplicationDescription(i18n("Akonadi Agent"));

    if (!parser.isSet(identifierOption)) {
        qCDebug(AKONADIAGENTBASE_LOG) << "Identifier argument missing";
        exit(1);
    }

    const QString identifier = parser.value(identifierOption);
    if (identifier.isEmpty()) {
        qCDebug(AKONADIAGENTBASE_LOG) << "Identifier argument is empty";
        exit(1);
    }

    QCoreApplication::setApplicationName(ServerManager::addNamespace(identifier));
    QCoreApplication::setApplicationVersion(QStringLiteral(AKONADI_FULL_VERSION));

    const QFileInfo fi(QString::fromLocal8Bit(argv[0]));
    // strip off full path and possible .exe suffix
    const QString catalog = fi.baseName();

    auto translator = new QTranslator(qApp);
    translator->load(catalog);
    QCoreApplication::installTranslator(translator);

    return identifier;
}

/// @endcond

int AgentBase::init(AgentBase &r)
{
    KLocalizedString::setApplicationDomain(QByteArrayLiteral("libakonadi6"));
    KAboutData::setApplicationData(r.aboutData());
    return qApp->exec();
}

int AgentBase::status() const
{
    Q_D(const AgentBase);

    return d->mStatusCode;
}

QString AgentBase::statusMessage() const
{
    Q_D(const AgentBase);

    return d->mStatusMessage;
}

int AgentBase::progress() const
{
    Q_D(const AgentBase);

    return d->mProgress;
}

QString AgentBase::progressMessage() const
{
    Q_D(const AgentBase);

    return d->mProgressMessage;
}

bool AgentBase::isOnline() const
{
    Q_D(const AgentBase);

    return d->mOnline;
}

void AgentBase::setNeedsNetwork(bool needsNetwork)
{
    Q_D(AgentBase);
    if (d->mNeedsNetwork == needsNetwork) {
        return;
    }

    d->mNeedsNetwork = needsNetwork;
    QNetworkInformation::load(QNetworkInformation::Feature::Reachability);
    connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, [this, d](auto reachability) {
        d->slotNetworkStatusChange(reachability == QNetworkInformation::Reachability::Online);
    });
}

void AgentBase::setOnline(bool state)
{
    Q_D(AgentBase);

    if (d->mPendingQuit) {
        return;
    }

    d->mDesiredOnlineState = state;
    if (!d->mSettings) {
        d->mSettings = new QSettings(ServerManager::agentConfigFilePath(identifier()), QSettings::IniFormat);
        d->mSettings->setValue(QStringLiteral("Agent/Name"), agentName());
    }
    d->mSettings->setValue(QStringLiteral("Agent/DesiredOnlineState"), state);
    setOnlineInternal(state);
}

void AgentBase::setTemporaryOffline(int makeOnlineInSeconds)
{
    Q_D(AgentBase);

    // if not currently online, avoid bringing it online after the timeout
    if (!d->mOnline) {
        return;
    }

    setOnlineInternal(false);

    if (!d->mTemporaryOfflineTimer) {
        d->mTemporaryOfflineTimer = new QTimer(d);
        d->mTemporaryOfflineTimer->setSingleShot(true);
        connect(d->mTemporaryOfflineTimer, &QTimer::timeout, d, &AgentBasePrivate::slotTemporaryOfflineTimeout);
    }
    d->mTemporaryOfflineTimer->setInterval(std::chrono::seconds{makeOnlineInSeconds});
    d->mTemporaryOfflineTimer->start();
}

void AgentBase::setOnlineInternal(bool state)
{
    Q_D(AgentBase);
    if (state && d->mNeedsNetwork) {
        if (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Online) {
            // Don't go online if the resource needs network but there is none
            state = false;
        }
    }
    d->mOnline = state;

    if (d->mTemporaryOfflineTimer) {
        d->mTemporaryOfflineTimer->stop();
    }

    const QString newMessage = d->defaultReadyMessage();
    if (d->mStatusMessage != newMessage && d->mStatusCode != AgentBase::Broken) {
        Q_EMIT status(d->mStatusCode, newMessage);
    }

    doSetOnline(state);
    Q_EMIT onlineChanged(state);
}

void AgentBase::doSetOnline(bool online)
{
    Q_UNUSED(online)
}

KAboutData AgentBase::aboutData() const
{
    // akonadi_google_resource_1 -> org.kde.akonadi_google_resource
    const QString desktopEntry = QLatin1String("org.kde.") + qApp->applicationName().remove(QRegularExpression(QStringLiteral("_[0-9]+$")));

    KAboutData data(qApp->applicationName(), agentName(), qApp->applicationVersion());
    data.setDesktopFileName(desktopEntry);
    return data;
}

void AgentBase::configure(WId windowId)
{
    Q_UNUSED(windowId)

    // Fallback if the agent implements the new plugin-based configuration,
    // but someone calls the deprecated configure() method
    auto instance = Akonadi::AgentManager::self()->instance(identifier());
    QPointer<AgentConfigurationDialog> dialog = new AgentConfigurationDialog(instance, nullptr);
    if (dialog->exec()) {
        Q_EMIT configurationDialogAccepted();
    } else {
        Q_EMIT configurationDialogRejected();
    }
    delete dialog;
}

#ifdef Q_OS_WIN // krazy:exclude=cpp
void AgentBase::configure(qlonglong windowId)
{
    configure(static_cast<WId>(windowId));
}
#endif

WId AgentBase::winIdForDialogs() const
{
    const bool registered = QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.akonaditray"));
    if (!registered) {
        return 0;
    }

    QDBusInterface dbus(QStringLiteral("org.freedesktop.akonaditray"), QStringLiteral("/Actions"), QStringLiteral("org.freedesktop.Akonadi.Tray"));
    const QDBusMessage reply = dbus.call(QStringLiteral("getWinId"));

    if (reply.type() == QDBusMessage::ErrorMessage) {
        return 0;
    }

    const WId winid = static_cast<WId>(reply.arguments().at(0).toLongLong());

    return winid;
}

void AgentBase::quit()
{
    Q_D(AgentBase);
    aboutToQuit();

    if (d->mSettings) {
        d->mChangeRecorder->setConfig(nullptr);
        d->mSettings->sync();
        delete d->mSettings;
        d->mSettings = nullptr;
    }

    delete d->mEventLoopLocker;
    d->mEventLoopLocker = nullptr;
}

void AgentBase::aboutToQuit()
{
    Q_D(AgentBase);
    d->mPendingQuit = true;
}

void AgentBase::cleanup()
{
    Q_D(AgentBase);
    // prevent the monitor from picking up deletion signals for our own data if we are a resource
    // and thus avoid that we kill our own data as last act before our own death
    d->mChangeRecorder->blockSignals(true);

    aboutToQuit();

    const QString fileName = d->mSettings->fileName();

    /*
     * First destroy the settings object...
     */
    d->mChangeRecorder->setConfig(nullptr);
    delete d->mSettings;
    d->mSettings = nullptr;

    /*
     * ... then remove the file from hd.
     */
    if (!QFile::remove(fileName)) {
        qCWarning(AKONADIAGENTBASE_LOG) << "Impossible to remove " << fileName;
    }

    /*
     * ... and remove the changes file from hd.
     */
    const QString changeDataFileName = fileName + QStringLiteral("_changes.dat");
    if (!QFile::remove(changeDataFileName)) {
        qCWarning(AKONADIAGENTBASE_LOG) << "Impossible to remove " << changeDataFileName;
    }

    /*
     * ... and also remove the agent configuration file if there is one.
     */
    const QString configFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1Char('/') + config()->name();
    if (!QFile::remove(configFile)) {
        qCWarning(AKONADIAGENTBASE_LOG) << "Impossible to remove config file " << configFile;
    }

    delete d->mEventLoopLocker;
    d->mEventLoopLocker = nullptr;
}

void AgentBase::registerObserver(Observer *observer)
{
    Q_D(AgentBase);

    // TODO in theory we should re-connect change recorder signals here that we disconnected previously
    d->mObserver = observer;

    const bool hasObserverV3 = (dynamic_cast<AgentBase::ObserverV3 *>(d->mObserver) != nullptr);
    const bool hasObserverV4 = (dynamic_cast<AgentBase::ObserverV4 *>(d->mObserver) != nullptr);

    disconnect(d->mChangeRecorder, &Monitor::tagAdded, d, &AgentBasePrivate::tagAdded);
    disconnect(d->mChangeRecorder, &Monitor::tagChanged, d, &AgentBasePrivate::tagChanged);
    disconnect(d->mChangeRecorder, &Monitor::tagRemoved, d, &AgentBasePrivate::tagRemoved);
    disconnect(d->mChangeRecorder, &Monitor::itemsTagsChanged, d, &AgentBasePrivate::itemsTagsChanged);
    disconnect(d->mChangeRecorder, &Monitor::itemsFlagsChanged, d, &AgentBasePrivate::itemsFlagsChanged);
    disconnect(d->mChangeRecorder, &Monitor::itemsMoved, d, &AgentBasePrivate::itemsMoved);
    disconnect(d->mChangeRecorder, &Monitor::itemsRemoved, d, &AgentBasePrivate::itemsRemoved);
    disconnect(d->mChangeRecorder, &Monitor::itemsLinked, d, &AgentBasePrivate::itemsLinked);
    disconnect(d->mChangeRecorder, &Monitor::itemsUnlinked, d, &AgentBasePrivate::itemsUnlinked);
    disconnect(d->mChangeRecorder, &Monitor::itemMoved, d, &AgentBasePrivate::itemMoved);
    disconnect(d->mChangeRecorder, &Monitor::itemRemoved, d, &AgentBasePrivate::itemRemoved);
    disconnect(d->mChangeRecorder, &Monitor::itemLinked, d, &AgentBasePrivate::itemLinked);
    disconnect(d->mChangeRecorder, &Monitor::itemUnlinked, d, &AgentBasePrivate::itemUnlinked);

    if (hasObserverV4) {
        connect(d->mChangeRecorder, &Monitor::tagAdded, d, &AgentBasePrivate::tagAdded);
        connect(d->mChangeRecorder, &Monitor::tagChanged, d, &AgentBasePrivate::tagChanged);
        connect(d->mChangeRecorder, &Monitor::tagRemoved, d, &AgentBasePrivate::tagRemoved);
        connect(d->mChangeRecorder, &Monitor::itemsTagsChanged, d, &AgentBasePrivate::itemsTagsChanged);
    }

    if (hasObserverV3) {
        connect(d->mChangeRecorder, &Monitor::itemsFlagsChanged, d, &AgentBasePrivate::itemsFlagsChanged);
        connect(d->mChangeRecorder, &Monitor::itemsMoved, d, &AgentBasePrivate::itemsMoved);
        connect(d->mChangeRecorder, &Monitor::itemsRemoved, d, &AgentBasePrivate::itemsRemoved);
        connect(d->mChangeRecorder, &Monitor::itemsLinked, d, &AgentBasePrivate::itemsLinked);
        connect(d->mChangeRecorder, &Monitor::itemsUnlinked, d, &AgentBasePrivate::itemsUnlinked);
    } else {
        // V2 - don't connect these if we have V3
        connect(d->mChangeRecorder, &Monitor::itemMoved, d, &AgentBasePrivate::itemMoved);
        connect(d->mChangeRecorder, &Monitor::itemRemoved, d, &AgentBasePrivate::itemRemoved);
        connect(d->mChangeRecorder, &Monitor::itemLinked, d, &AgentBasePrivate::itemLinked);
        connect(d->mChangeRecorder, &Monitor::itemUnlinked, d, &AgentBasePrivate::itemUnlinked);
    }
}

QString AgentBase::identifier() const
{
    return d_ptr->mId;
}

void AgentBase::setAgentName(const QString &name)
{
    Q_D(AgentBase);
    if (name == d->mName) {
        return;
    }

    // TODO: rename collection
    d->mName = name;

    if (d->mName.isEmpty() || d->mName == d->mId) {
        d->mSettings->remove(QStringLiteral("Resource/Name"));
        d->mSettings->remove(QStringLiteral("Agent/Name"));
    } else {
        d->mSettings->setValue(QStringLiteral("Agent/Name"), d->mName);
    }

    d->mSettings->sync();

    d->setProgramName();

    Q_EMIT agentNameChanged(d->mName);
}

QString AgentBase::agentName() const
{
    Q_D(const AgentBase);
    if (d->mName.isEmpty()) {
        return d->mId;
    } else {
        return d->mName;
    }
}

void AgentBase::changeProcessed()
{
    Q_D(AgentBase);
    d->changeProcessed();
}

ChangeRecorder *AgentBase::changeRecorder() const
{
    return d_ptr->mChangeRecorder;
}

KSharedConfigPtr AgentBase::config()
{
    return KSharedConfig::openConfig();
}

void AgentBase::abort()
{
    Q_EMIT abortRequested();
}

void AgentBase::reconfigure()
{
    Q_EMIT reloadConfiguration();
}

#include "moc_agentbase.cpp"
#include "moc_agentbase_p.cpp"
