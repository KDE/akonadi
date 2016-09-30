/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "servermanager.h"
#include "servermanager_p.h"

#include "agenttype.h"
#include "agentmanager.h"
#include "KDBusConnectionPool"
#include "session_p.h"
#include "firstrun_p.h"

#include "akonadicore_debug.h"

#include <Kdelibs4ConfigMigrator>

#include "private/protocol_p.h"
#include "private/standarddirs_p.h"
#include "private/dbus_p.h"
#include "private/instance_p.h"

#include <QtDBus>
#include <QTimer>
#include <QScopedPointer>

using namespace Akonadi;

class Akonadi::ServerManagerPrivate
{
public:
    ServerManagerPrivate()
        : instance(new ServerManager(this))
        , mState(ServerManager::NotRunning)
        , mSafetyTimer(new QTimer)
        , mFirstRunner(0)
    {
        mState = instance->state();
        mSafetyTimer->setSingleShot(true);
        mSafetyTimer->setInterval(30000);
        QObject::connect(mSafetyTimer.data(), SIGNAL(timeout()), instance, SLOT(timeout()));
        if (mState == ServerManager::Running && Internal::clientType() == Internal::User && !ServerManager::hasInstanceIdentifier()) {
            mFirstRunner = new Firstrun(instance);
        }
    }

    ~ServerManagerPrivate()
    {
        delete instance;
    }

    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
    {
        if (name == ServerManager::serviceName(ServerManager::ControlLock) && !oldOwner.isEmpty() && newOwner.isEmpty()) {
            // Control.Lock has disappeared during startup, which means that akonadi_control
            // has terminated, most probably because it was not able to start akonadiserver
            // process. Don't wait 30 seconds for sefetyTimeout, but go into Broken state
            // immediately.
            if (mState == ServerManager::Starting) {
                setState(ServerManager::Broken);
                return;
            }
        }

        serverProtocolVersion = -1,
        checkStatusChanged();
    }

    void checkStatusChanged()
    {
        setState(instance->state());
    }

    void setState(ServerManager::State state)
    {

        if (mState != state) {
            mState = state;
            emit instance->stateChanged(state);
            if (state == ServerManager::Running) {
                emit instance->started();
                if (!mFirstRunner && Internal::clientType() == Internal::User && !ServerManager::hasInstanceIdentifier()) {
                    mFirstRunner = new Firstrun(instance);
                }
            } else if (state == ServerManager::NotRunning || state == ServerManager::Broken) {
                emit instance->stopped();
            }

            if (state == ServerManager::Starting || state == ServerManager::Stopping) {
                QMetaObject::invokeMethod(mSafetyTimer.data(), "start", Qt::QueuedConnection);   // in case we are in a different thread
            } else {
                QMetaObject::invokeMethod(mSafetyTimer.data(), "stop", Qt::QueuedConnection);   // in case we are in a different thread
            }
        }
    }

    void timeout()
    {
        if (mState == ServerManager::Starting || mState == ServerManager::Stopping) {
            setState(ServerManager::Broken);
        }
    }

    ServerManager *instance;
    static int serverProtocolVersion;
    static uint generation;
    ServerManager::State mState;
    QScopedPointer<QTimer> mSafetyTimer;
    Firstrun *mFirstRunner;
    static Internal::ClientType clientType;
};

int ServerManagerPrivate::serverProtocolVersion = -1;
uint ServerManagerPrivate::generation = 0;
Internal::ClientType ServerManagerPrivate::clientType = Internal::User;

Q_GLOBAL_STATIC(ServerManagerPrivate, sInstance)

ServerManager::ServerManager(ServerManagerPrivate *dd)
    : d(dd)
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("servermanager"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("akonadi-firstrunrc"));
    migrate.migrate();

    qRegisterMetaType<Akonadi::ServerManager::State>();

    QDBusServiceWatcher *watcher = new QDBusServiceWatcher(ServerManager::serviceName(ServerManager::Server),
            KDBusConnectionPool::threadConnection(),
            QDBusServiceWatcher::WatchForOwnerChange, this);
    watcher->addWatchedService(ServerManager::serviceName(ServerManager::Control));
    watcher->addWatchedService(ServerManager::serviceName(ServerManager::ControlLock));
    watcher->addWatchedService(ServerManager::serviceName(ServerManager::UpgradeIndicator));

    // this (and also the two connects below) are queued so that they trigger after AgentManager is done loading
    // the current agent types and instances
    // this ensures the invariant of AgentManager reporting a consistent state if ServerManager::state() == Running
    // that's the case with direct connections as well, but only after you enter the event loop once
    connect(watcher, SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)), Qt::QueuedConnection);

    // AgentManager is dangerous to use for agents themselves
    if (Internal::clientType() != Internal::User) {
        return;
    }
    connect(AgentManager::self(), SIGNAL(typeAdded(Akonadi::AgentType)), SLOT(checkStatusChanged()), Qt::QueuedConnection);
    connect(AgentManager::self(), SIGNAL(typeRemoved(Akonadi::AgentType)), SLOT(checkStatusChanged()), Qt::QueuedConnection);
}

ServerManager *Akonadi::ServerManager::self()
{
    return sInstance->instance;
}

bool ServerManager::start()
{
    const bool controlRegistered = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Control));
    const bool serverRegistered = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Server));
    if (controlRegistered && serverRegistered) {
        return true;
    }

    const bool controlLockRegistered = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::ControlLock));
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
        QDBusReply<void> reply = KDBusConnectionPool::threadConnection().interface()->startService(ServerManager::serviceName(ServerManager::Control));
        if (!reply.isValid()) {
            qCDebug(AKONADICORE_LOG) << "Akonadi server could not be started via D-Bus either: "
                     << reply.error().message();
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
    Q_UNUSED(parent);
    QProcess::startDetached(QStringLiteral("akonadiselftest"));
}

bool ServerManager::isRunning()
{
    return state() == Running;
}

ServerManager::State ServerManager::state()
{
    ServerManager::State previousState = NotRunning;
    if (sInstance.exists()) {   // be careful, this is called from the ServerManager::Private ctor, so using sInstance unprotected can cause infinite recursion
        previousState = sInstance->mState;
    }

    const bool serverUpgrading = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::UpgradeIndicator));
    if (serverUpgrading) {
        return Upgrading;
    }

    const bool controlRegistered = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Control));
    const bool serverRegistered = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Server));
    if (controlRegistered && serverRegistered) {
        // check if the server protocol is recent enough
        if (sInstance.exists()) {
            if (Internal::serverProtocolVersion() >= 0 &&
                    Internal::serverProtocolVersion() < Protocol::version()) {
                return Broken;
            }
        }

        // AgentManager is dangerous to use for agents themselves
        if (Internal::clientType() == Internal::User) {
            // besides the running server processes we also need at least one resource to be operational
            AgentType::List agentTypes = AgentManager::self()->types();
            foreach (const AgentType &type, agentTypes) {
                if (type.capabilities().contains(QStringLiteral("Resource"))) {
                    return Running;
                }
            }
            return Broken;
        } else {
            return Running;
        }
    }

    const bool controlLockRegistered = KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::ControlLock));
    if (controlLockRegistered || controlRegistered) {
        qCDebug(AKONADICORE_LOG) << "Akonadi server is already starting up";
        if (previousState == Running) {
            return NotRunning; // we don't know if it's starting or stopping, probably triggered by someone else
        }
        return previousState;
    }

    if (serverRegistered) {
        qCWarning(AKONADICORE_LOG) << "Akonadi server running without control process!";
        return Broken;
    }

    if (previousState == Starting) {   // valid case where nothing is running (yet)
        return previousState;
    }
    return NotRunning;
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
    QString relPath;
    if (hasInstanceIdentifier()) {
        relPath = QStringLiteral("instance/%1").arg(ServerManager::instanceIdentifier());
    }
    return XdgBaseDirs::akonadiServerConfigFile(openMode == Akonadi::ServerManager::ReadOnly
                                                    ? XdgBaseDirs::ReadOnly
                                                    : XdgBaseDirs::ReadWrite,
                                                relPath);
}

QString ServerManager::agentConfigFilePath(const QString &identifier)
{
    QString fullRelPath = QStringLiteral("akonadi");
    if (hasInstanceIdentifier()) {
        fullRelPath += QStringLiteral("/instance/%1").arg(ServerManager::instanceIdentifier());
    }
    fullRelPath += QStringLiteral("agent_config_%1").arg(identifier);
    return Akonadi::XdgBaseDirs::findResourceFile("config", fullRelPath);
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
