/*
    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>
    Copyright (c) 2008 Kevin Krammer <kevin.krammer@gmx.at>

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

#include "agentbase.h"
#include "agentbase_p.h"

#include "akonadi_version.h"
#include "agentmanager.h"
#include "changerecorder.h"
#include "controladaptor.h"
#include "KDBusConnectionPool"
#include "itemfetchjob.h"
#include "monitor_p.h"
#include "servermanager_p.h"
#include "session.h"
#include "session_p.h"
#include "statusadaptor.h"
#include "private/standarddirs_p.h"


#include "akonadiagentbase_debug.h"

#include <KLocalizedString>
#include <KSharedConfig>

#include <Kdelibs4ConfigMigrator>

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtDBus/QtDBus>
#include <QApplication>
#include <QtCore/QCommandLineParser>
#include <QtNetwork/QNetworkConfiguration>
#include <QNetworkConfigurationManager>

#include <signal.h>
#include <stdlib.h>
#include <QStandardPaths>
#if defined __GLIBC__
# include <malloc.h> // for dumping memory information
#endif

//#define EXPERIMENTAL_INPROCESS_AGENTS 1

using namespace Akonadi;

static AgentBase *sAgentBase = 0;

AgentBase::Observer::Observer()
{
}

AgentBase::Observer::~Observer()
{
}

void AgentBase::Observer::itemAdded(const Item &item, const Collection &collection)
{
    Q_UNUSED(item);
    Q_UNUSED(collection);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::itemChanged(const Item &item, const QSet<QByteArray> &partIdentifiers)
{
    Q_UNUSED(item);
    Q_UNUSED(partIdentifiers);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::itemRemoved(const Item &item)
{
    Q_UNUSED(item);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(collection);
    Q_UNUSED(parent);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::collectionChanged(const Collection &collection)
{
    Q_UNUSED(collection);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::Observer::collectionRemoved(const Collection &collection)
{
    Q_UNUSED(collection);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    Q_UNUSED(item);
    Q_UNUSED(source);
    Q_UNUSED(dest);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(item);
    Q_UNUSED(collection);
    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemLinked,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemLinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    Q_UNUSED(item);
    Q_UNUSED(collection);
    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemUnlinked,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemUnlinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    Q_UNUSED(collection);
    Q_UNUSED(source);
    Q_UNUSED(dest);
    if (sAgentBase != 0) {
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV2::collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes)
{
    Q_UNUSED(changedAttributes);
    collectionChanged(collection);
}

void AgentBase::ObserverV3::itemsFlagsChanged(const Akonadi::Item::List &items, const QSet< QByteArray > &addedFlags, const QSet< QByteArray > &removedFlags)
{
    Q_UNUSED(items);
    Q_UNUSED(addedFlags);
    Q_UNUSED(removedFlags);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsFlagsChanged,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemsFlagsChanged);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsMoved(const Akonadi::Item::List &items, const Collection &sourceCollection, const Collection &destinationCollection)
{
    Q_UNUSED(items);
    Q_UNUSED(sourceCollection);
    Q_UNUSED(destinationCollection);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsMoved,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemsMoved);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsRemoved(const Akonadi::Item::List &items)
{
    Q_UNUSED(items);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsRemoved,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemsRemoved);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsLinked(const Akonadi::Item::List &items, const Collection &collection)
{
    Q_UNUSED(items);
    Q_UNUSED(collection);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsLinked,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemsLinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV3::itemsUnlinked(const Akonadi::Item::List &items, const Collection &collection)
{
    Q_UNUSED(items);
    Q_UNUSED(collection)

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimizations in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsUnlinked,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemsUnlinked);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::tagAdded(const Tag &tag)
{
    Q_UNUSED(tag);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::tagAdded,
                            sAgentBase->d_ptr, &AgentBasePrivate::tagAdded);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::tagChanged(const Tag &tag)
{
    Q_UNUSED(tag);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::tagChanged,
                            sAgentBase->d_ptr, &AgentBasePrivate::tagChanged);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::tagRemoved(const Tag &tag)
{
    Q_UNUSED(tag);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::tagRemoved,
                            sAgentBase->d_ptr, &AgentBasePrivate::tagRemoved);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::itemsTagsChanged(const Item::List &items, const QSet<Tag> &addedTags, const QSet<Tag> &removedTags)
{
    Q_UNUSED(items);
    Q_UNUSED(addedTags);
    Q_UNUSED(removedTags);

    if (sAgentBase != 0) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::itemsTagsChanged,
                            sAgentBase->d_ptr, &AgentBasePrivate::itemsTagsChanged);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::relationAdded(const Akonadi::Relation &relation)
{
    Q_UNUSED(relation)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::relationAdded,
                            sAgentBase->d_ptr, &AgentBasePrivate::relationAdded);
        sAgentBase->d_ptr->changeProcessed();
    }
}

void AgentBase::ObserverV4::relationRemoved(const Akonadi::Relation &relation)
{
    Q_UNUSED(relation)

    if (sAgentBase) {
        // not implementation, let's disconnect the signal to enable optimization in Monitor
        QObject::disconnect(sAgentBase->changeRecorder(), &Monitor::relationRemoved,
                            sAgentBase->d_ptr, &AgentBasePrivate::relationRemoved);
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
        QObject::disconnect(sAgentBase->changeRecorder(), SIGNAL(itemsRelationsChanged(Akonadi::Item::List,Akonadi::Relation::List,Akonadi::Relation::List)),
                            sAgentBase, SLOT(itemsRelationsChanged(Akonadi::Item::List,Akonadi::Relation::List,Akonadi::Relation::List)));
        sAgentBase->d_ptr->changeProcessed();
    }
}

//@cond PRIVATE

AgentBasePrivate::AgentBasePrivate(AgentBase *parent)
    : q_ptr(parent)
    , mStatusCode(AgentBase::Idle)
    , mProgress(0)
    , mNeedsNetwork(false)
    , mOnline(false)
    , mDesiredOnlineState(false)
    , mSettings(0)
    , mChangeRecorder(0)
    , mTracer(0)
    , mObserver(0)
    , mPowerInterface(0)
    , mTemporaryOfflineTimer(0)
    , mEventLoopLocker(0)
    , mNetworkManager(Q_NULLPTR)
{
    Internal::setClientType(Internal::Agent);
}

AgentBasePrivate::~AgentBasePrivate()
{
    mChangeRecorder->setConfig(0);
    delete mSettings;
}

void AgentBasePrivate::init()
{
    Q_Q(AgentBase);

    Kdelibs4ConfigMigrator migrate(mId);
    migrate.setConfigFiles(QStringList() << QStringLiteral("%1rc").arg(mId));
    migrate.migrate();

    /**
     * Create a default session for this process.
     */
    SessionPrivate::createDefaultSession(mId.toLatin1());

    mTracer = new org::freedesktop::Akonadi::Tracer(ServerManager::serviceName(ServerManager::Server),
            QStringLiteral("/tracing"),
            KDBusConnectionPool::threadConnection(), q);

    new Akonadi__ControlAdaptor(q);
    new Akonadi__StatusAdaptor(q);
    if (!KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/"), q, QDBusConnection::ExportAdaptors)) {
        q->error(i18n("Unable to register object at dbus: %1", KDBusConnectionPool::threadConnection().lastError().message()));
    }

    mSettings = new QSettings(QStringLiteral("%1/agent_config_%2").arg(StandardDirs::saveDir("config"), mId), QSettings::IniFormat);

    mChangeRecorder = new ChangeRecorder(q);
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

    connect(mChangeRecorder, &Monitor::itemAdded,
            this, &AgentBasePrivate::itemAdded);
    connect(mChangeRecorder, &Monitor::itemChanged,
            this, &AgentBasePrivate::itemChanged);
    connect(mChangeRecorder, &Monitor::collectionAdded,
            this, &AgentBasePrivate::collectionAdded);
    connect(mChangeRecorder, SIGNAL(collectionChanged(Akonadi::Collection)),
            SLOT(collectionChanged(Akonadi::Collection)));
    connect(mChangeRecorder, SIGNAL(collectionChanged(Akonadi::Collection,QSet<QByteArray>)),
            SLOT(collectionChanged(Akonadi::Collection,QSet<QByteArray>)));
    connect(mChangeRecorder, &Monitor::collectionMoved,
            this, &AgentBasePrivate::collectionMoved);
    connect(mChangeRecorder, &Monitor::collectionRemoved,
            this, &AgentBasePrivate::collectionRemoved);
    connect(mChangeRecorder, &Monitor::collectionSubscribed,
            this, &AgentBasePrivate::collectionSubscribed);
    connect(mChangeRecorder, &Monitor::collectionUnsubscribed,
            this, &AgentBasePrivate::collectionUnsubscribed);

    connect(q, SIGNAL(status(int,QString)), q, SLOT(slotStatus(int,QString)));
    connect(q, SIGNAL(percent(int)), q, SLOT(slotPercent(int)));
    connect(q, SIGNAL(warning(QString)), q, SLOT(slotWarning(QString)));
    connect(q, SIGNAL(error(QString)), q, SLOT(slotError(QString)));

    mPowerInterface = new QDBusInterface(QStringLiteral("org.kde.Solid.PowerManagement"),
                                         QStringLiteral("/org/kde/Solid/PowerManagement/Actions/SuspendSession"),
                                         QStringLiteral("org.kde.Solid.PowerManagement.Actions.SuspendSession"),
                                         QDBusConnection::sessionBus(), this);
    if (mPowerInterface->isValid()) {
        connect(mPowerInterface, SIGNAL(resumingFromSuspend()),
                q, SLOT(slotResumedFromSuspend()));
    } else {
        delete mPowerInterface;
        mPowerInterface = 0;
    }

    // Use reference counting to allow agents to finish internal jobs when the
    // agent is stopped.
    mEventLoopLocker = new QEventLoopLocker();

    mResourceTypeName = AgentManager::self()->instance(mId).type().name();
    setProgramName();

    QTimer::singleShot(0, q, SLOT(delayedInit()));
}

void AgentBasePrivate::delayedInit()
{
    Q_Q(AgentBase);

    const QString serviceId = ServerManager::agentServiceName(ServerManager::Agent, mId);
    if (!KDBusConnectionPool::threadConnection().registerService(serviceId)) {
        qCritical() << "Unable to register service" << serviceId << "at dbus:" << KDBusConnectionPool::threadConnection().lastError().message();
    }
    q->setOnlineInternal(mDesiredOnlineState);

    KDBusConnectionPool::threadConnection().registerObject(QStringLiteral("/Debug"), this, QDBusConnection::ExportScriptableSlots);
}

void AgentBasePrivate::setProgramName()
{
    // ugly, really ugly, if you find another solution, change it and blame me for this code (Andras)
    QString programName = mResourceTypeName;
    if (!mName.isEmpty()) {
        programName = i18nc("Name and type of Akonadi resource", "%1 of type %2", mName, mResourceTypeName) ;
    }

    QGuiApplication::setApplicationDisplayName(programName);
}

void AgentBasePrivate::itemAdded(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    if (mObserver != 0) {
        mObserver->itemAdded(item, collection);
    }
}

void AgentBasePrivate::itemChanged(const Akonadi::Item &item, const QSet<QByteArray> &partIdentifiers)
{
    if (mObserver != 0) {
        mObserver->itemChanged(item, partIdentifiers);
    }
}

void AgentBasePrivate::itemMoved(const Akonadi::Item &item, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    AgentBase::ObserverV2 *observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (mObserver) {
        // inter-resource moves, requires we know which resources the source and destination are in though
        if (!source.resource().isEmpty() && !dest.resource().isEmpty()) {
            if (source.resource() != dest.resource()) {
                if (source.resource() == q_ptr->identifier()) {   // moved away from us
                    Akonadi::Item i(item);
                    i.setParentCollection(source);
                    mObserver->itemRemoved(i);
                } else if (dest.resource() == q_ptr->identifier()) {   // moved to us
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
    if (mObserver != 0) {
        mObserver->itemRemoved(item);
    }
}

void AgentBasePrivate::itemLinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    AgentBase::ObserverV2 *observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (observer2) {
        observer2->itemLinked(item, collection);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemUnlinked(const Akonadi::Item &item, const Akonadi::Collection &collection)
{
    AgentBase::ObserverV2 *observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (observer2) {
        observer2->itemUnlinked(item, collection);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemsFlagsChanged(const Akonadi::Item::List &items, const QSet<QByteArray> &addedFlags, const QSet<QByteArray> &removedFlags)
{
    AgentBase::ObserverV3 *observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsFlagsChanged(items, addedFlags, removedFlags);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::itemsMoved(const Akonadi::Item::List &items, const Akonadi::Collection &source, const Akonadi::Collection &destination)
{
    AgentBase::ObserverV3 *observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsMoved(items, source, destination);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::itemsRemoved(const Akonadi::Item::List &items)
{
    AgentBase::ObserverV3 *observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
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

    AgentBase::ObserverV3 *observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsLinked(items, collection);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::itemsUnlinked(const Akonadi::Item::List &items, const Akonadi::Collection &collection)
{
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV3 *observer3 = dynamic_cast<AgentBase::ObserverV3 *>(mObserver);
    if (observer3) {
        observer3->itemsUnlinked(items, collection);
    } else {
        Q_ASSERT_X(false, Q_FUNC_INFO, "Batch slots must never be called when ObserverV3 is not available");
    }
}

void AgentBasePrivate::tagAdded(const Akonadi::Tag &tag)
{
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV4 *observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->tagAdded(tag);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::tagChanged(const Akonadi::Tag &tag)
{
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV4 *observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->tagChanged(tag);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::tagRemoved(const Akonadi::Tag &tag)
{
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV4 *observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->tagRemoved(tag);;
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::itemsTagsChanged(const Akonadi::Item::List &items, const QSet<Akonadi::Tag> &addedTags, const QSet<Akonadi::Tag> &removedTags)
{
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV4 *observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->itemsTagsChanged(items, addedTags, removedTags);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::relationAdded(const Akonadi::Relation &relation)
{
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV4 *observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->relationAdded(relation);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::relationRemoved(const Akonadi::Relation &relation)
{
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV4 *observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
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
    if (!mObserver) {
        return;
    }

    AgentBase::ObserverV4 *observer4 = dynamic_cast<AgentBase::ObserverV4 *>(mObserver);
    if (observer4) {
        observer4->itemsRelationsChanged(items, addedRelations, removedRelations);
    } else {
        changeProcessed();
    }
}

void AgentBasePrivate::collectionAdded(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    if (mObserver != 0) {
        mObserver->collectionAdded(collection, parent);
    }
}

void AgentBasePrivate::collectionChanged(const Akonadi::Collection &collection)
{
    AgentBase::ObserverV2 *observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (mObserver != 0 && observer2 == 0) {   // For ObserverV2 we use the variant with the part identifiers
        mObserver->collectionChanged(collection);
    }
}

void AgentBasePrivate::collectionChanged(const Akonadi::Collection &collection, const QSet<QByteArray> &changedAttributes)
{
    AgentBase::ObserverV2 *observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
    if (observer2 != 0) {
        observer2->collectionChanged(collection, changedAttributes);
    }
}

void AgentBasePrivate::collectionMoved(const Akonadi::Collection &collection, const Akonadi::Collection &source, const Akonadi::Collection &dest)
{
    AgentBase::ObserverV2 *observer2 = dynamic_cast<AgentBase::ObserverV2 *>(mObserver);
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
    if (mObserver != 0) {
        mObserver->collectionRemoved(collection);
    }
}

void AgentBasePrivate::collectionSubscribed(const Akonadi::Collection &collection, const Akonadi::Collection &parent)
{
    Q_UNUSED(collection);
    Q_UNUSED(parent);
    changeProcessed();
}

void AgentBasePrivate::collectionUnsubscribed(const Akonadi::Collection &collection)
{
    Q_UNUSED(collection);
    changeProcessed();
}

void AgentBasePrivate::changeProcessed()
{
    mChangeRecorder->changeProcessed();
    QTimer::singleShot(0, mChangeRecorder, &ChangeRecorder::replayNext);
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
    Q_UNUSED(isOnline);
    Q_Q(AgentBase);
    q->setOnlineInternal(mDesiredOnlineState);
}

void AgentBasePrivate::slotResumedFromSuspend()
{
    if (mNeedsNetwork) {
        slotNetworkStatusChange(mNetworkManager->isOnline());
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
    stream
            << "Total non-mmapped bytes (arena):      " << mi.arena     << '\n'
            << "# of free chunks (ordblks):           " << mi.ordblks   << '\n'
            << "# of free fastbin blocks (smblks>:    " << mi.smblks    << '\n'
            << "# of mapped regions (hblks):          " << mi.hblks     << '\n'
            << "Bytes in mapped regions (hblkhd):     " << mi.hblkhd    << '\n'
            << "Max. total allocated space (usmblks): " << mi.usmblks   << '\n'
            << "Free bytes held in fastbins (fsmblks):" << mi.fsmblks   << '\n'
            << "Total allocated space (uordblks):     " << mi.uordblks  << '\n'
            << "Total free space (fordblks):          " << mi.fordblks  << '\n'
            << "Topmost releasable block (keepcost):  " << mi.keepcost  << '\n';
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

AgentBase::~AgentBase()
{
    delete d_ptr;
}

QString AgentBase::parseArguments(int argc, char **argv)
{
    Q_UNUSED(argc);

    QCommandLineOption identifierOption(QStringLiteral("identifier"), i18n("Agent identifier"),
                                        QStringLiteral("argument"));
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
    QCoreApplication::setApplicationVersion(QStringLiteral(AKONADILIBRARIES_VERSION_STRING));

    const QFileInfo fi(QString::fromLocal8Bit(argv[0]));
    // strip off full path and possible .exe suffix
    const QString catalog = fi.baseName();

    QTranslator *translator = new QTranslator();
    translator->load(catalog);
    QCoreApplication::installTranslator(translator);

    return identifier;
}

// @endcond

int AgentBase::init(AgentBase *r)
{
    KLocalizedString::setApplicationDomain("libakonadi5");
    int rv = qApp->exec();
    delete r;
    return rv;
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

    if (d->mNeedsNetwork) {
        d->mNetworkManager = new QNetworkConfigurationManager(this);
        connect(d->mNetworkManager, SIGNAL(onlineStateChanged(bool)),
                this, SLOT(slotNetworkStatusChange(bool)),
                Qt::UniqueConnection);

    } else {
        delete d->mNetworkManager;
        setOnlineInternal(d->mDesiredOnlineState);
    }
}

void AgentBase::setOnline(bool state)
{
    Q_D(AgentBase);
    d->mDesiredOnlineState = state;
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
        connect(d->mTemporaryOfflineTimer, SIGNAL(timeout()), this, SLOT(slotTemporaryOfflineTimeout()));
    }
    d->mTemporaryOfflineTimer->setInterval(makeOnlineInSeconds * 1000);
    d->mTemporaryOfflineTimer->start();
}

void AgentBase::setOnlineInternal(bool state)
{
    Q_D(AgentBase);
    if (state && d->mNeedsNetwork) {
        if (!d->mNetworkManager->isOnline()) {
            //Don't go online if the resource needs network but there is none
            state = false;
        }
    }
    d->mOnline = state;

    if (d->mTemporaryOfflineTimer) {
        d->mTemporaryOfflineTimer->stop();
    }

    const QString newMessage = d->defaultReadyMessage();
    if (d->mStatusMessage != newMessage && d->mStatusCode != AgentBase::Broken) {
        emit status(d->mStatusCode, newMessage);
    }

    doSetOnline(state);
    emit onlineChanged(state);
}

void AgentBase::doSetOnline(bool online)
{
    Q_UNUSED(online);
}

void AgentBase::configure(WId windowId)
{
    Q_UNUSED(windowId);
    emit configurationDialogAccepted();
}

#ifdef Q_OS_WIN //krazy:exclude=cpp
void AgentBase::configure(qlonglong windowId)
{
    configure(reinterpret_cast<WId>(windowId));
}
#endif

WId AgentBase::winIdForDialogs() const
{
    const bool registered = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.akonaditray"));
    if (!registered) {
        return 0;
    }

    QDBusInterface dbus(QStringLiteral("org.freedesktop.akonaditray"), QStringLiteral("/Actions"),
                        QStringLiteral("org.freedesktop.Akonadi.Tray"));
    const QDBusMessage reply = dbus.call(QStringLiteral("getWinId"));

    if (reply.type() == QDBusMessage::ErrorMessage) {
        return 0;
    }

    const WId winid = (WId)reply.arguments().at(0).toLongLong();

    return winid;
}

void AgentBase::quit()
{
    Q_D(AgentBase);
    aboutToQuit();

    if (d->mSettings) {
        d->mChangeRecorder->setConfig(0);
        d->mSettings->sync();
    }

    delete d->mEventLoopLocker;
    d->mEventLoopLocker = Q_NULLPTR;
}

void AgentBase::aboutToQuit()
{
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
    d->mChangeRecorder->setConfig(0);
    delete d->mSettings;
    d->mSettings = 0;

    /*
     * ... then remove the file from hd.
     */
    QFile::remove(fileName);

    /*
     * ... and remove the changes file from hd.
     */
    QFile::remove(fileName + QStringLiteral("_changes.dat"));

    /*
     * ... and also remove the agent configuration file if there is one.
     */
    QString configFile = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QLatin1Char('/') + config()->name();
    QFile::remove(configFile);

    delete d->mEventLoopLocker;
    d->mEventLoopLocker = Q_NULLPTR;
}

void AgentBase::registerObserver(Observer *observer)
{
    // TODO in theory we should re-connect change recorder signals here that we disconnected previously
    d_ptr->mObserver = observer;

    const bool hasObserverV3 = (dynamic_cast<AgentBase::ObserverV3 *>(d_ptr->mObserver) != 0);
    const bool hasObserverV4 = (dynamic_cast<AgentBase::ObserverV4 *>(d_ptr->mObserver) != 0);

    disconnect(d_ptr->mChangeRecorder, &Monitor::tagAdded,
               d_ptr, &AgentBasePrivate::tagAdded);
    disconnect(d_ptr->mChangeRecorder, &Monitor::tagChanged,
               d_ptr, &AgentBasePrivate::tagChanged);
    disconnect(d_ptr->mChangeRecorder, &Monitor::tagRemoved,
               d_ptr, &AgentBasePrivate::tagRemoved);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemsTagsChanged,
               d_ptr, &AgentBasePrivate::itemsTagsChanged);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemsFlagsChanged,
               d_ptr, &AgentBasePrivate::itemsFlagsChanged);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemsMoved,
               d_ptr, &AgentBasePrivate::itemsMoved);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemsRemoved,
               d_ptr, &AgentBasePrivate::itemsRemoved);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemsLinked,
               d_ptr, &AgentBasePrivate::itemsLinked);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemsUnlinked,
               d_ptr, &AgentBasePrivate::itemsUnlinked);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemMoved,
               d_ptr, &AgentBasePrivate::itemMoved);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemRemoved,
               d_ptr, &AgentBasePrivate::itemRemoved);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemLinked,
               d_ptr, &AgentBasePrivate::itemLinked);
    disconnect(d_ptr->mChangeRecorder, &Monitor::itemUnlinked,
               d_ptr, &AgentBasePrivate::itemUnlinked);

    if (hasObserverV4) {
        connect(d_ptr->mChangeRecorder, &Monitor::tagAdded,
                d_ptr, &AgentBasePrivate::tagAdded);
        connect(d_ptr->mChangeRecorder, &Monitor::tagChanged,
                d_ptr, &AgentBasePrivate::tagChanged);
        connect(d_ptr->mChangeRecorder, &Monitor::tagRemoved,
                d_ptr, &AgentBasePrivate::tagRemoved);
        connect(d_ptr->mChangeRecorder, &Monitor::itemsTagsChanged,
                d_ptr, &AgentBasePrivate::itemsTagsChanged);
    }

    if (hasObserverV3) {
        connect(d_ptr->mChangeRecorder, &Monitor::itemsFlagsChanged,
                d_ptr, &AgentBasePrivate::itemsFlagsChanged);
        connect(d_ptr->mChangeRecorder, &Monitor::itemsMoved,
                d_ptr, &AgentBasePrivate::itemsMoved);
        connect(d_ptr->mChangeRecorder, &Monitor::itemsRemoved,
                d_ptr, &AgentBasePrivate::itemsRemoved);
        connect(d_ptr->mChangeRecorder, &Monitor::itemsLinked,
                d_ptr, &AgentBasePrivate::itemsLinked);
        connect(d_ptr->mChangeRecorder, &Monitor::itemsUnlinked,
                d_ptr, &AgentBasePrivate::itemsUnlinked);
    } else {
        // V2 - don't connect these if we have V3
        connect(d_ptr->mChangeRecorder, &Monitor::itemMoved,
                d_ptr, &AgentBasePrivate::itemMoved);
        connect(d_ptr->mChangeRecorder, &Monitor::itemRemoved,
                d_ptr, &AgentBasePrivate::itemRemoved);
        connect(d_ptr->mChangeRecorder, &Monitor::itemLinked,
                d_ptr, &AgentBasePrivate::itemLinked);
        connect(d_ptr->mChangeRecorder, &Monitor::itemUnlinked,
                d_ptr, &AgentBasePrivate::itemUnlinked);
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

    emit agentNameChanged(d->mName);
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
    emit abortRequested();
}

void AgentBase::reconfigure()
{
    emit reloadConfiguration();
}

#include "moc_agentbase.cpp"
#include "moc_agentbase_p.cpp"
