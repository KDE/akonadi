/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "servermanager.h"
#include "servermanager_p.h"

#include "agentmanager.h"
#include "agenttype.h"
#include "firstrun_p.h"
#include "session_p.h"

#include "akonadicore_debug.h"

#include <KLocalizedString>
#include <kcoreaddons_version.h>
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <Kdelibs4ConfigMigrator>
#endif

#include "private/dbus_p.h"
#include "private/instance_p.h"
#include "private/protocol_p.h"
#include "private/standarddirs_p.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <QProcess>
#include <QScopedPointer>
#include <QTimer>
#include <qnamespace.h>

using namespace Akonadi;

class Akonadi::ServerManagerPrivate
{
public:
    ServerManagerPrivate()
        : instance(new ServerManager(this))
        , mState(ServerManager::NotRunning)
        , mSafetyTimer(new QTimer)
    {
        mState = instance->state();
        mSafetyTimer->setSingleShot(true);
        mSafetyTimer->setInterval(30000);
        QObject::connect(mSafetyTimer.data(), &QTimer::timeout, instance, [this]() {
            timeout();
        });
        if (mState == ServerManager::Running && Internal::clientType() == Internal::User && !ServerManager::hasInstanceIdentifier()) {
            mFirstRunner = new Firstrun(instance);
        }
    }

    ~ServerManagerPrivate()
    {
        delete instance;
    }

    void checkStatusChanged()
    {
        setState(instance->state());
    }

    void setState(ServerManager::State state)
    {
        if (mState != state) {
            mState = state;
            Q_EMIT instance->stateChanged(state);
            if (state == ServerManager::Running) {
                Q_EMIT instance->started();
                if (!mFirstRunner && Internal::clientType() == Internal::User && !ServerManager::hasInstanceIdentifier()) {
                    mFirstRunner = new Firstrun(instance);
                }
            } else if (state == ServerManager::NotRunning || state == ServerManager::Broken) {
                Q_EMIT instance->stopped();
            }
            if (state == ServerManager::Starting || state == ServerManager::Stopping) {
                QMetaObject::invokeMethod(mSafetyTimer.data(), QOverload<>::of(&QTimer::start), Qt::QueuedConnection); // in case we are in a different thread
            } else {
                QMetaObject::invokeMethod(mSafetyTimer.data(), &QTimer::stop, Qt::QueuedConnection); // in case we are in a different thread
            }
        }
    }

    void timeout()
    {
        if (mState == ServerManager::Starting || mState == ServerManager::Stopping) {
            setState(ServerManager::Broken);
        }
    }

    ServerManager *instance = nullptr;
    static int serverProtocolVersion;
    static uint generation;
    ServerManager::State mState;
    QScopedPointer<QTimer> mSafetyTimer;
    Firstrun *mFirstRunner = nullptr;
    static Internal::ClientType clientType;
    QString mBrokenReason;
    std::unique_ptr<QDBusServiceWatcher> serviceWatcher;
};

int ServerManagerPrivate::serverProtocolVersion = -1;
uint ServerManagerPrivate::generation = 0;
Internal::ClientType ServerManagerPrivate::clientType = Internal::User;

Q_GLOBAL_STATIC(ServerManagerPrivate, sInstance) // NOLINT(readability-redundant-member-init)

ServerManager::ServerManager(ServerManagerPrivate *dd)
    : d(dd)
{
#if KCOREADDONS_VERSION < QT_VERSION_CHECK(6, 0, 0)
    Kdelibs4ConfigMigrator migrate(QStringLiteral("servermanager"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi-firstrunrc"));
    migrate.migrate();
#endif
    qRegisterMetaType<Akonadi::ServerManager::State>();

    d->serviceWatcher = std::make_unique<QDBusServiceWatcher>(ServerManager::serviceName(ServerManager::Server),
                                                              QDBusConnection::sessionBus(),
                                                              QDBusServiceWatcher::WatchForRegistration | QDBusServiceWatcher::WatchForUnregistration);
    d->serviceWatcher->addWatchedService(ServerManager::serviceName(ServerManager::Control));
    d->serviceWatcher->addWatchedService(ServerManager::serviceName(ServerManager::ControlLock));
    d->serviceWatcher->addWatchedService(ServerManager::serviceName(ServerManager::UpgradeIndicator));

    // this (and also the two connects below) are queued so that they trigger after AgentManager is done loading
    // the current agent types and instances
    // this ensures the invariant of AgentManager reporting a consistent state if ServerManager::state() == Running
    // that's the case with direct connections as well, but only after you enter the event loop once
    connect(
        d->serviceWatcher.get(),
        &QDBusServiceWatcher::serviceRegistered,
        this,
        [this]() {
            d->serverProtocolVersion = -1;
            d->checkStatusChanged();
        },
        Qt::QueuedConnection);
    connect(
        d->serviceWatcher.get(),
        &QDBusServiceWatcher::serviceUnregistered,
        this,
        [this](const QString &name) {
            if (name == ServerManager::serviceName(ServerManager::ControlLock) && d->mState == ServerManager::Starting) {
                // Control.Lock has disappeared during startup, which means that akonadi_control
                // has terminated, most probably because it was not able to start akonadiserver
                // process. Don't wait 30 seconds for sefetyTimeout, but go into Broken state
                // immediately.
                d->setState(ServerManager::Broken);
                return;
            }

            d->serverProtocolVersion = -1;
            d->checkStatusChanged();
        },
        Qt::QueuedConnection);

    // AgentManager is dangerous to use for agents themselves
    if (Internal::clientType() != Internal::User) {
        return;
    }

    connect(
        AgentManager::self(),
        &AgentManager::typeAdded,
        this,
        [this]() {
            d->checkStatusChanged();
        },
        Qt::QueuedConnection);
    connect(
        AgentManager::self(),
        &AgentManager::typeRemoved,
        this,
        [this]() {
            d->checkStatusChanged();
        },
        Qt::QueuedConnection);
}

ServerManager *Akonadi::ServerManager::self()
{
    return sInstance->instance;
}

bool ServerManager::start()
{
    const bool controlRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Control));
    const bool serverRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Server));
    if (controlRegistered && serverRegistered) {
        return true;
    }

    const bool controlLockRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::ControlLock));
    if (controlLockRegistered || controlRegistered) {
        qCDebug(AKONADICORE_LOG) << "Akonadi server is already starting up";
        sInstance->setState(Starting);
        return true;
    }

    qCDebug(AKONADICORE_LOG) << "executing akonadi_control";
    QStringList args;
    if (hasInstanceIdentifier()) {
        args << QStringLiteral("--instance") << instanceIdentifier();
    }
    const bool ok = QProcess::startDetached(QStringLiteral("akonadi_control"), args);
    if (!ok) {
        qCWarning(AKONADICORE_LOG) << "Unable to execute akonadi_control, falling back to D-Bus auto-launch";
        QDBusReply<void> reply = QDBusConnection::sessionBus().interface()->startService(ServerManager::serviceName(ServerManager::Control));
        if (!reply.isValid()) {
            qCDebug(AKONADICORE_LOG) << "Akonadi server could not be started via D-Bus either: " << reply.error().message();
            return false;
        }
    }
    sInstance->setState(Starting);
    return true;
}

bool ServerManager::stop()
{
    QDBusInterface iface(ServerManager::serviceName(ServerManager::Control),
                         QStringLiteral("/ControlManager"),
                         QStringLiteral("org.freedesktop.Akonadi.ControlManager"));
    if (!iface.isValid()) {
        return false;
    }
    iface.call(QDBus::NoBlock, QStringLiteral("shutdown"));
    sInstance->setState(Stopping);
    return true;
}

void ServerManager::showSelfTestDialog(QWidget *parent)
{
    Q_UNUSED(parent)
    QProcess::startDetached(QStringLiteral("akonadiselftest"), QStringList());
}

bool ServerManager::isRunning()
{
    return state() == Running;
}

ServerManager::State ServerManager::state()
{
    ServerManager::State previousState = NotRunning;
    if (sInstance.exists()) { // be careful, this is called from the ServerManager::Private ctor, so using sInstance unprotected can cause infinite recursion
        previousState = sInstance->mState;
        sInstance->mBrokenReason.clear();
    }

    const bool serverUpgrading = QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::UpgradeIndicator));
    if (serverUpgrading) {
        return Upgrading;
    }

    const bool controlRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Control));
    const bool serverRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Server));
    if (controlRegistered && serverRegistered) {
        // check if the server protocol is recent enough
        if (sInstance.exists()) {
            if (Internal::serverProtocolVersion() >= 0 && Internal::serverProtocolVersion() != Protocol::version()) {
                sInstance->mBrokenReason = i18n(
                    "The Akonadi server protocol version differs from the protocol version used by this application.\n"
                    "If you recently updated your system please log out and back in to make sure all applications use the "
                    "correct protocol version.");
                return Broken;
            }
        }

        // AgentManager is dangerous to use for agents themselves
        if (Internal::clientType() == Internal::User) {
            // besides the running server processes we also need at least one resource to be operational
            const AgentType::List agentTypes = AgentManager::self()->types();
            for (const AgentType &type : agentTypes) {
                if (type.capabilities().contains(QLatin1String("Resource"))) {
                    return Running;
                }
            }
            if (sInstance.exists()) {
                sInstance->mBrokenReason = i18n("There are no Akonadi Agents available. Please verify your KDE PIM installation.");
            }
            return Broken;
        } else {
            return Running;
        }
    }

    const bool controlLockRegistered = QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::ControlLock));
    if (controlLockRegistered || controlRegistered) {
        qCDebug(AKONADICORE_LOG) << "Akonadi server is only partially running. Server:" << serverRegistered << "ControlLock:" << controlLockRegistered
                                 << "Control:" << controlRegistered;
        if (previousState == Running) {
            return NotRunning; // we don't know if it's starting or stopping, probably triggered by someone else
        }
        return previousState;
    }

    if (serverRegistered) {
        qCWarning(AKONADICORE_LOG) << "Akonadi server running without control process!";
        return Broken;
    }

    if (previousState == Starting) { // valid case where nothing is running (yet)
        return previousState;
    }
    return NotRunning;
}

QString ServerManager::brokenReason()
{
    if (sInstance.exists()) {
        return sInstance->mBrokenReason;
    }
    return QString();
}

QString ServerManager::instanceIdentifier()
{
    return Instance::identifier();
}

bool ServerManager::hasInstanceIdentifier()
{
    return Instance::hasIdentifier();
}

QString ServerManager::serviceName(ServerManager::ServiceType serviceType)
{
    switch (serviceType) {
    case Server:
        return DBus::serviceName(DBus::Server);
    case Control:
        return DBus::serviceName(DBus::Control);
    case ControlLock:
        return DBus::serviceName(DBus::ControlLock);
    case UpgradeIndicator:
        return DBus::serviceName(DBus::UpgradeIndicator);
    }
    Q_ASSERT(!"WTF?");
    return QString();
}

QString ServerManager::agentServiceName(ServiceAgentType agentType, const QString &identifier)
{
    switch (agentType) {
    case Agent:
        return DBus::agentServiceName(identifier, DBus::Agent);
    case Resource:
        return DBus::agentServiceName(identifier, DBus::Resource);
    case Preprocessor:
        return DBus::agentServiceName(identifier, DBus::Preprocessor);
    }
    Q_ASSERT(!"WTF?");
    return QString();
}

QString ServerManager::serverConfigFilePath(OpenMode openMode)
{
    return StandardDirs::serverConfigFile(openMode == Akonadi::ServerManager::ReadOnly ? StandardDirs::ReadOnly : StandardDirs::ReadWrite);
}

QString ServerManager::agentConfigFilePath(const QString &identifier)
{
    return StandardDirs::agentConfigFile(identifier);
}

QString ServerManager::addNamespace(const QString &string)
{
    if (Instance::hasIdentifier()) {
        return string % QLatin1Char('_') % Instance::identifier();
    }
    return string;
}

uint ServerManager::generation()
{
    return Internal::generation();
}

int Internal::serverProtocolVersion()
{
    return ServerManagerPrivate::serverProtocolVersion;
}

void Internal::setServerProtocolVersion(int version)
{
    ServerManagerPrivate::serverProtocolVersion = version;
    if (sInstance.exists()) {
        sInstance->checkStatusChanged();
    }
}

uint Internal::generation()
{
    return ServerManagerPrivate::generation;
}

void Internal::setGeneration(uint generation)
{
    ServerManagerPrivate::generation = generation;
}

Internal::ClientType Internal::clientType()
{
    return ServerManagerPrivate::clientType;
}

void Internal::setClientType(ClientType type)
{
    ServerManagerPrivate::clientType = type;
}

#include "moc_servermanager.cpp"
