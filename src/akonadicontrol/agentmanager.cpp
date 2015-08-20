/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (c) 2007 Volker Krause <vkrause@kde.org>                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "agentmanager.h"

#include "agentmanageradaptor.h"
#include "agentmanagerinternaladaptor.h"
#include "agentprocessinstance.h"
#include "agentserverinterface.h"
#include "agentthreadinstance.h"
#include "preprocessor_manager.h"
#include "processcontrol.h"
#include "resource_manager.h"
#include "serverinterface.h"

#include <private/protocol_p.h>
#include <private/xdgbasedirs_p.h>
#include <private/instance_p.h>
#include <private/standarddirs_p.h>

#include <shared/akdebug.h>
#include <shared/akdbus.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#ifndef QT_NO_DEBUG
#include <QtCore/QFileSystemWatcher>
#endif
#include <QScopedPointer>
#include <QtCore/QSettings>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

using Akonadi::ProcessControl;

static const bool enableAgentServerDefault = false;

AgentManager::AgentManager(QObject *parent)
    : QObject(parent)
    , mAgentServer(0)
#ifndef QT_NO_DEBUG
    , mAgentWatcher(new QFileSystemWatcher(this))
#endif
{
    new AgentManagerAdaptor(this);
    new AgentManagerInternalAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/AgentManager"), this);

    connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(AkDBus::serviceName(AkDBus::Server))) {
        akFatal() << "akonadiserver already running!";
    }

    const QSettings settings(Akonadi::StandardDirs::agentConfigFile(Akonadi::XdgBaseDirs::ReadOnly), QSettings::IniFormat);
    mAgentServerEnabled = settings.value(QStringLiteral("AgentServer/Enabled"), enableAgentServerDefault).toBool();

    QStringList serviceArgs;
    if (Akonadi::Instance::hasIdentifier()) {
        serviceArgs << QStringLiteral("--instance") << Akonadi::Instance::identifier();
    }

    mStorageController = new Akonadi::ProcessControl;
    mStorageController->setShutdownTimeout(15 * 1000);   // the server needs more time for shutdown if we are using an internal mysqld
    connect(mStorageController, SIGNAL(unableToStart()), SLOT(serverFailure()));
    mStorageController->start(QStringLiteral("akonadiserver"), serviceArgs, Akonadi::ProcessControl::RestartOnCrash);

    if (mAgentServerEnabled) {
        mAgentServer = new Akonadi::ProcessControl;
        connect(mAgentServer, SIGNAL(unableToStart()), SLOT(agentServerFailure()));
        mAgentServer->start(QStringLiteral("akonadi_agent_server"), serviceArgs, Akonadi::ProcessControl::RestartOnCrash);
    }

#ifndef QT_NO_DEBUG
    connect(mAgentWatcher, SIGNAL(fileChanged(QString)), SLOT(agentExeChanged(QString)));
#endif
}

void AgentManager::continueStartup()
{
    // prevent multiple calls in case the server has to be restarted
    static bool first = true;
    if (!first) {
        return;
    }

    first = false;

    readPluginInfos();
    Q_FOREACH (const AgentType &info, mAgents) {
        Q_EMIT agentTypeAdded(info.identifier);
    }

    const QStringList pathList = pluginInfoPathList();

#ifndef QT_NO_DEBUG
    Q_FOREACH (const QString &path, pathList) {
        QFileSystemWatcher *watcher = new QFileSystemWatcher(this);
        watcher->addPath(path);

        connect(watcher, SIGNAL(directoryChanged(QString)),
                this, SLOT(updatePluginInfos()));
    }
#endif

    load();
    Q_FOREACH (const AgentType &info, mAgents) {
        ensureAutoStart(info);
    }

    // register the real service name once everything is up an running
    if (!QDBusConnection::sessionBus().registerService(AkDBus::serviceName(AkDBus::Control))) {
        // besides a race with an older Akonadi server I have no idea how we could possibly get here...
        akFatal() << "Unable to register service as" << AkDBus::serviceName(AkDBus::Control)
                  << "despite having the lock. Error was:" << QDBusConnection::sessionBus().lastError().message();
    }
    akDebug() << "Akonadi server is now operational.";
}

AgentManager::~AgentManager()
{
    cleanup();
}

void AgentManager::cleanup()
{
    Q_FOREACH (const AgentInstance::Ptr &instance, mAgentInstances) {
        instance->quit();
    }

    mAgentInstances.clear();

    mStorageController->setCrashPolicy(ProcessControl::StopOnCrash);
    org::freedesktop::Akonadi::Server *serverIface =
        new org::freedesktop::Akonadi::Server(AkDBus::serviceName(AkDBus::Server), QStringLiteral("/Server"),
                                              QDBusConnection::sessionBus(), this);
    serverIface->quit();

    if (mAgentServer) {
        mAgentServer->setCrashPolicy(ProcessControl::StopOnCrash);
        org::freedesktop::Akonadi::AgentServer *agentServerIface =
            new org::freedesktop::Akonadi::AgentServer(AkDBus::serviceName(AkDBus::AgentServer),
                                                       QStringLiteral("/AgentServer"), QDBusConnection::sessionBus(), this);
        agentServerIface->quit();
    }

    delete mStorageController;
    mStorageController = 0;

    delete mAgentServer;
    mAgentServer = 0;
}

QStringList AgentManager::agentTypes() const
{
    return mAgents.keys();
}

QString AgentManager::agentName(const QString &identifier, const QString &language) const
{
    if (!checkAgentExists(identifier)) {
        return QString();
    }

    const QString name = mAgents.value(identifier).name.value(language);
    return name.isEmpty() ? mAgents.value(identifier).name.value(QStringLiteral("en_US")) : name;
}

QString AgentManager::agentComment(const QString &identifier, const QString &language) const
{
    if (!checkAgentExists(identifier)) {
        return QString();
    }

    const QString comment = mAgents.value(identifier).comment.value(language);
    return comment.isEmpty() ? mAgents.value(identifier).comment.value(QStringLiteral("en_US")) : comment;
}

QString AgentManager::agentIcon(const QString &identifier) const
{
    if (!checkAgentExists(identifier)) {
        return QString();
    }

    const AgentType info = mAgents.value(identifier);
    if (!info.icon.isEmpty()) {
        return info.icon;
    }

    return QStringLiteral("application-x-executable");
}

QStringList AgentManager::agentMimeTypes(const QString &identifier) const
{
    if (!checkAgentExists(identifier)) {
        return QStringList();
    }

    return mAgents.value(identifier).mimeTypes;
}

QStringList AgentManager::agentCapabilities(const QString &identifier) const
{
    if (!checkAgentExists(identifier)) {
        return QStringList();
    }
    return mAgents.value(identifier).capabilities;
}

QVariantMap AgentManager::agentCustomProperties(const QString &identifier) const
{
    if (!checkAgentExists(identifier)) {
        return QVariantMap();
    }

    return mAgents.value(identifier).custom;
}

AgentInstance::Ptr AgentManager::createAgentInstance(const AgentType &info)
{
    switch (info.launchMethod) {
    case AgentType::Server:
        return AgentInstance::Ptr(new Akonadi::AgentThreadInstance(this));
    case AgentType::Launcher: // Fall through
    case AgentType::Process:
        return AgentInstance::Ptr(new Akonadi::AgentProcessInstance(this));
    default:
        Q_ASSERT_X(false, "AgentManger::createAgentInstance", "Unhandled AgentType::LaunchMethod case");
    }

    return AgentInstance::Ptr();
}

QString AgentManager::createAgentInstance(const QString &identifier)
{
    if (!checkAgentExists(identifier)) {
        return QString();
    }

    const AgentType agentInfo = mAgents.value(identifier);
    mAgents[identifier].instanceCounter++;

    const AgentInstance::Ptr instance = createAgentInstance(agentInfo);
    if (agentInfo.capabilities.contains(AgentType::CapabilityUnique)) {
        instance->setIdentifier(identifier);
    } else {
        instance->setIdentifier(QStringLiteral("%1_%2").arg(identifier, QString::number(agentInfo.instanceCounter)));
    }

    if (mAgentInstances.contains(instance->identifier())) {
        akError() << Q_FUNC_INFO << "Cannot create another instance of agent" << identifier;
        return QString();
    }

    // Return from this dbus call before we do the next. Otherwise dbus brakes for
    // this process.
    if (calledFromDBus()) {
        connection().send(message().createReply(instance->identifier()));
    }

    if (!instance->start(agentInfo)) {
        return QString();
    }

    mAgentInstances.insert(instance->identifier(), instance);
    registerAgentAtServer(instance->identifier(), agentInfo);
    save();

    return instance->identifier();
}

void AgentManager::removeAgentInstance(const QString &identifier)
{
    if (!mAgentInstances.contains(identifier)) {
        akError() << Q_FUNC_INFO << "Agent instance with identifier" << identifier << "does not exist";
        return;
    }

    const AgentInstance::Ptr instance = mAgentInstances.value(identifier);
    if (instance->hasAgentInterface()) {
        instance->cleanup();
    } else {
        akError() << Q_FUNC_INFO << "Agent instance" << identifier << "has no interface!";
    }

    mAgentInstances.remove(identifier);

    save();

    org::freedesktop::Akonadi::ResourceManager resmanager(AkDBus::serviceName(AkDBus::Server), QStringLiteral("/ResourceManager"), QDBusConnection::sessionBus(), this);
    resmanager.removeResourceInstance(instance->identifier());

    // Kill the preprocessor instance, if any.
    org::freedesktop::Akonadi::PreprocessorManager preProcessorManager(
        AkDBus::serviceName(AkDBus::Server),
        QStringLiteral("/PreprocessorManager"),
        QDBusConnection::sessionBus(),
        this);

    preProcessorManager.unregisterInstance(instance->identifier());

    if (instance->hasAgentInterface()) {
        akDebug() << "AgentManager::removeAgentInstance: calling instance->quit()";
        instance->quit();
    } else {
        akError() << Q_FUNC_INFO << "Agent instance" << identifier << "has no interface!";
    }

    Q_EMIT agentInstanceRemoved(identifier);
}

QString AgentManager::agentInstanceType(const QString &identifier)
{
    if (!mAgentInstances.contains(identifier)) {
        akError() << Q_FUNC_INFO << "Agent instance with identifier" << identifier << "does not exist";
        return QString();
    }

    return mAgentInstances.value(identifier)->agentType();
}

QStringList AgentManager::agentInstances() const
{
    return mAgentInstances.keys();
}

int AgentManager::agentInstanceStatus(const QString &identifier) const
{
    if (!checkInstance(identifier)) {
        return 2;
    }

    return mAgentInstances.value(identifier)->status();
}

QString AgentManager::agentInstanceStatusMessage(const QString &identifier) const
{
    if (!checkInstance(identifier)) {
        return QString();
    }

    return mAgentInstances.value(identifier)->statusMessage();
}

uint AgentManager::agentInstanceProgress(const QString &identifier) const
{
    if (!checkInstance(identifier)) {
        return 0;
    }

    return mAgentInstances.value(identifier)->progress();
}

QString AgentManager::agentInstanceProgressMessage(const QString &identifier) const
{
    Q_UNUSED(identifier);

    return QString();
}

void AgentManager::agentInstanceConfigure(const QString &identifier, qlonglong windowId)
{
    if (!checkAgentInterfaces(identifier, QStringLiteral("agentInstanceConfigure"))) {
        return;
    }

    mAgentInstances.value(identifier)->configure(windowId);
}

bool AgentManager::agentInstanceOnline(const QString &identifier)
{
    if (!checkInstance(identifier)) {
        return false;
    }

    return mAgentInstances.value(identifier)->isOnline();
}

void AgentManager::setAgentInstanceOnline(const QString &identifier, bool state)
{
    if (!checkAgentInterfaces(identifier, QStringLiteral("setAgentInstanceOnline"))) {
        return;
    }

    mAgentInstances.value(identifier)->statusInterface()->setOnline(state);
}

// resource specific methods //
void AgentManager::setAgentInstanceName(const QString &identifier, const QString &name)
{
    if (!checkResourceInterface(identifier, QStringLiteral("setAgentInstanceName"))) {
        return;
    }

    mAgentInstances.value(identifier)->resourceInterface()->setName(name);
}

QString AgentManager::agentInstanceName(const QString &identifier, const QString &language) const
{
    if (!checkInstance(identifier)) {
        return QString();
    }

    const AgentInstance::Ptr instance = mAgentInstances.value(identifier);
    if (!instance->resourceName().isEmpty()) {
        return instance->resourceName();
    }

    if (!checkAgentExists(instance->agentType())) {
        return QString();
    }

    const QString name = mAgents.value(instance->agentType()).name.value(language);
    return name.isEmpty() ? mAgents.value(instance->agentType()).name.value(QStringLiteral("en_US")) : name;
}

void AgentManager::agentInstanceSynchronize(const QString &identifier)
{
    if (!checkResourceInterface(identifier, QStringLiteral("agentInstanceSynchronize"))) {
        return;
    }

    mAgentInstances.value(identifier)->resourceInterface()->synchronize();
}

void AgentManager::agentInstanceSynchronizeCollectionTree(const QString &identifier)
{
    if (!checkResourceInterface(identifier, QStringLiteral("agentInstanceSynchronizeCollectionTree"))) {
        return;
    }

    mAgentInstances.value(identifier)->resourceInterface()->synchronizeCollectionTree();
}

void AgentManager::agentInstanceSynchronizeCollection(const QString &identifier, qint64 collection)
{
    agentInstanceSynchronizeCollection(identifier, collection, false);
}

void AgentManager::agentInstanceSynchronizeCollection(const QString &identifier, qint64 collection, bool recursive)
{
    if (!checkResourceInterface(identifier, QStringLiteral("agentInstanceSynchronizeCollection"))) {
        return;
    }

    mAgentInstances.value(identifier)->resourceInterface()->synchronizeCollection(collection, recursive);
}

void AgentManager::restartAgentInstance(const QString &identifier)
{
    if (!checkInstance(identifier)) {
        return;
    }

    mAgentInstances.value(identifier)->restartWhenIdle();
}

void AgentManager::updatePluginInfos()
{
    const QHash<QString, AgentType> oldInfos = mAgents;
    readPluginInfos();

    Q_FOREACH (const AgentType &oldInfo, oldInfos) {
        if (!mAgents.contains(oldInfo.identifier)) {
            Q_EMIT agentTypeRemoved(oldInfo.identifier);
        }
    }

    Q_FOREACH (const AgentType &newInfo, mAgents) {
        if (!oldInfos.contains(newInfo.identifier)) {
            Q_EMIT agentTypeAdded(newInfo.identifier);
            ensureAutoStart(newInfo);
        }
    }
}

void AgentManager::readPluginInfos()
{
#ifndef QT_NO_DEBUG
    if (!mAgentWatcher->files().isEmpty()) {
        mAgentWatcher->removePaths(mAgentWatcher->files());
    }
#endif
    mAgents.clear();

    const QStringList pathList = pluginInfoPathList();

    Q_FOREACH (const QString &path, pathList) {
        const QDir directory(path, QStringLiteral("*.desktop"));
        readPluginInfos(directory);
    }
}

void AgentManager::readPluginInfos(const QDir &directory)
{
    const QStringList files = directory.entryList();
    akDebug() << "PLUGINS: " << directory.canonicalPath();
    akDebug() << "PLUGINS: " << files;

    for (int i = 0; i < files.count(); ++i) {
        const QString fileName = directory.absoluteFilePath(files[i]);

        AgentType agentInfo;
        if (agentInfo.load(fileName, this)) {
            if (mAgents.contains(agentInfo.identifier)) {
                akError() << Q_FUNC_INFO << "Duplicated agent identifier" << agentInfo.identifier << "from file" << fileName;
                continue;
            }

            const QString disableAutostart = getEnv("AKONADI_DISABLE_AGENT_AUTOSTART");
            if (!disableAutostart.isEmpty()) {
                akDebug() << "Autostarting of agents is disabled.";
                agentInfo.capabilities.removeOne(AgentType::CapabilityAutostart);
            }

            if (!mAgentServerEnabled && agentInfo.launchMethod == AgentType::Server) {
                agentInfo.launchMethod = AgentType::Launcher;
            }

            if (agentInfo.launchMethod == AgentType::Process) {
                const QString executable = Akonadi::XdgBaseDirs::findExecutableFile(agentInfo.exec);
                if (executable.isEmpty()) {
                    akError() << "Executable" << agentInfo.exec << "for agent" << agentInfo.identifier << "could not be found!";
                    continue;
                }
#ifndef QT_NO_DEBUG
                if (!mAgentWatcher->files().contains(executable)) {
                    mAgentWatcher->addPath(executable);
                }
#endif
            }

            akDebug() << "PLUGINS inserting: " << agentInfo.identifier << agentInfo.instanceCounter << agentInfo.capabilities;
            mAgents.insert(agentInfo.identifier, agentInfo);
        }
    }
}

QStringList AgentManager::pluginInfoPathList()
{
    return Akonadi::XdgBaseDirs::findAllResourceDirs("data", QStringLiteral("akonadi/agents"));
}

void AgentManager::load()
{
    org::freedesktop::Akonadi::ResourceManager resmanager(AkDBus::serviceName(AkDBus::Server), QStringLiteral("/ResourceManager"), QDBusConnection::sessionBus(), this);
    const QStringList knownResources = resmanager.resourceInstances();

    QSettings file(Akonadi::StandardDirs::agentConfigFile(Akonadi::XdgBaseDirs::ReadOnly), QSettings::IniFormat);
    file.beginGroup(QStringLiteral("Instances"));
    const QStringList entries = file.childGroups();
    for (int i = 0; i < entries.count(); ++i) {
        const QString instanceIdentifier = entries[i];

        if (mAgentInstances.contains(instanceIdentifier)) {
            akError() << Q_FUNC_INFO << "Duplicated instance identifier" << instanceIdentifier << "found in agentsrc";
            continue;
        }

        file.beginGroup(entries[i]);

        const QString agentType = file.value(QStringLiteral("AgentType")).toString();
        if (!mAgents.contains(agentType)) {
            akError() << Q_FUNC_INFO << "Reference to unknown agent type" << agentType << "in agentsrc";
            file.endGroup();
            continue;
        }
        const AgentType type = mAgents.value(agentType);

        // recover if the db has been deleted in the meantime or got otherwise corrupted
        if (!knownResources.contains(instanceIdentifier) && type.capabilities.contains(AgentType::CapabilityResource)) {
            akDebug() << "Recovering instance" << instanceIdentifier << "after database loss";
            registerAgentAtServer(instanceIdentifier, type);
        }

        const AgentInstance::Ptr instance = createAgentInstance(type);
        instance->setIdentifier(instanceIdentifier);
        if (instance->start(type)) {
            mAgentInstances.insert(instanceIdentifier, instance);
        }

        file.endGroup();
    }

    file.endGroup();
}

void AgentManager::save()
{
    QSettings file(Akonadi::StandardDirs::agentConfigFile(Akonadi::XdgBaseDirs::WriteOnly), QSettings::IniFormat);

    Q_FOREACH (const AgentType &info, mAgents) {
        info.save(&file);
    }

    file.beginGroup(QStringLiteral("Instances"));
    file.remove(QString());
    Q_FOREACH (const AgentInstance::Ptr &instance, mAgentInstances) {
        file.beginGroup(instance->identifier());
        file.setValue(QStringLiteral("AgentType"), instance->agentType());
        file.endGroup();
    }

    file.endGroup();
}

void AgentManager::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner);
    // This is called by the D-Bus server when a service comes up, goes down or changes ownership for some reason
    // and this is where we "hook up" our different Agent interfaces.

    //akDebug() << "Service " << name << " owner changed from " << oldOwner << " to " << newOwner;

    if ((name == AkDBus::serviceName(AkDBus::Server) || name == AkDBus::serviceName(AkDBus::AgentServer)) && !newOwner.isEmpty()) {
        if (QDBusConnection::sessionBus().interface()->isServiceRegistered(AkDBus::serviceName(AkDBus::Server))
            && (!mAgentServer || QDBusConnection::sessionBus().interface()->isServiceRegistered(AkDBus::serviceName(AkDBus::AgentServer)))) {
            // server is operational, start agents
            continueStartup();
        }
    }

    AkDBus::AgentType agentType = AkDBus::Unknown;
    const QString agentIdentifier = AkDBus::parseAgentServiceName(name, agentType);
    switch (agentType) {
    case AkDBus::Agent: {
        // An agent service went up or down
        if (newOwner.isEmpty()) {
            return; // It went down: we don't care here.
        }

        if (!mAgentInstances.contains(agentIdentifier)) {
            return;
        }

        const AgentInstance::Ptr instance = mAgentInstances.value(agentIdentifier);
        const bool restarting = instance->hasAgentInterface();
        if (!instance->obtainAgentInterface()) {
            return;
        }

        if (!restarting) {
            Q_EMIT agentInstanceAdded(agentIdentifier);
        }

        break;
    }
    case AkDBus::Resource: {
        // A resource service went up or down
        if (newOwner.isEmpty()) {
            return; // It went down: we don't care here.
        }

        if (!mAgentInstances.contains(agentIdentifier)) {
            return;
        }

        mAgentInstances.value(agentIdentifier)->obtainResourceInterface();

        break;
    }
    case AkDBus::Preprocessor: {
       // A preprocessor service went up or down

       // If the preprocessor is going up then the org.freedesktop.Akonadi.Agent.* interface
       // should be already up (as it's registered before the preprocessor one).
       // So if we don't know about the preprocessor as agent instance
       // then it's not our preprocessor.

       // If the preprocessor is going down then either the agent interface already
       // went down (and it has been already unregistered on the manager side)
       // or it's still registered as agent and WE have to unregister it.
       // The order of interface deletions depends on Qt but we handle both cases.

       // Check if we "know" about it.
       akDebug() << "Preprocessor " << agentIdentifier << " is going up or down...";

       if (!mAgentInstances.contains(agentIdentifier)) {
           akDebug() << "But it isn't registered as agent... not mine (anymore?)";
           return; // not our agent (?)
       }

       org::freedesktop::Akonadi::PreprocessorManager preProcessorManager(
           AkDBus::serviceName(AkDBus::Server),
           QStringLiteral("/PreprocessorManager"),
           QDBusConnection::sessionBus(),
           this);

       if (!preProcessorManager.isValid()) {
           akError() << Q_FUNC_INFO << "Could not connect to PreprocessorManager via D-Bus:" << preProcessorManager.lastError().message();
       } else {
           if (newOwner.isEmpty()) {
               // The preprocessor went down. Unregister it on server side.

               preProcessorManager.unregisterInstance(agentIdentifier);

           } else {

               // The preprocessor went up. Register it on server side.

               if (!mAgentInstances.value(agentIdentifier)->obtainPreprocessorInterface()) {
                   // Hm.. couldn't hook up its preprocessor interface..
                   // Make sure we don't have it in the preprocessor chain
                   qWarning() << "Couldn't obtain preprocessor interface for instance" << agentIdentifier;

                   preProcessorManager.unregisterInstance(agentIdentifier);
                   return;
               }

               akDebug() << "Registering preprocessor instance" << agentIdentifier;

               // Add to the preprocessor chain
               preProcessorManager.registerInstance(agentIdentifier);
           }
       }

       break;
    }
    default:
        break;
    }
}

bool AgentManager::checkInstance(const QString &identifier) const
{
    if (!mAgentInstances.contains(identifier)) {
        qWarning() << "Agent instance with identifier " << identifier << " does not exist";
        return false;
    }

    return true;
}

bool AgentManager::checkResourceInterface(const QString &identifier, const QString &method) const
{
    if (!checkInstance(identifier)) {
        return false;
    }

    if (!mAgents[mAgentInstances[identifier]->agentType()].capabilities.contains(QStringLiteral("Resource"))) {
        return false;
    }

    if (!mAgentInstances[identifier]->hasResourceInterface()) {
        qWarning() << QLatin1String("AgentManager::") + method << " Agent instance "
                   << identifier << " has no resource interface!";
        return false;
    }

    return true;
}

bool AgentManager::checkAgentExists(const QString &identifier) const
{
    if (!mAgents.contains(identifier)) {
        qWarning() << "Agent instance " << identifier << " does not exist.";
        return false;
    }

    return true;
}

bool AgentManager::checkAgentInterfaces(const QString &identifier, const QString &method) const
{
    if (!checkInstance(identifier)) {
        return false;
    }

    if (!mAgentInstances.value(identifier)->hasAgentInterface()) {
        qWarning() << "Agent instance (" << method << ") " << identifier << " has no agent interface.";
        return false;
    }

    return true;
}

void AgentManager::ensureAutoStart(const AgentType &info)
{
    if (!info.capabilities.contains(AgentType::CapabilityAutostart)) {
        return; // no an autostart agent
    }

    org::freedesktop::Akonadi::AgentServer agentServer(AkDBus::serviceName(AkDBus::AgentServer),
                                                       QStringLiteral("/AgentServer"), QDBusConnection::sessionBus(), this);

    if (mAgentInstances.contains(info.identifier) ||
        (agentServer.isValid() && agentServer.started(info.identifier))) {
        return; // already running
    }

    const AgentInstance::Ptr instance = createAgentInstance(info);
    instance->setIdentifier(info.identifier);
    if (instance->start(info)) {
        mAgentInstances.insert(instance->identifier(), instance);
        registerAgentAtServer(instance->identifier(), info);
        save();
    }
}

void AgentManager::agentExeChanged(const QString &fileName)
{
    if (!QFile::exists(fileName)) {
        return;
    }

    Q_FOREACH (const AgentType &type, mAgents) {
        if (fileName.endsWith(type.exec)) {
            Q_FOREACH (const AgentInstance::Ptr &instance, mAgentInstances) {
                if (instance->agentType() == type.identifier) {
                    instance->restartWhenIdle();
                }
            }
        }
    }
}

void AgentManager::registerAgentAtServer(const QString &agentIdentifier, const AgentType &type)
{
    if (type.capabilities.contains(AgentType::CapabilityResource)) {
        QScopedPointer<org::freedesktop::Akonadi::ResourceManager> resmanager(
            new org::freedesktop::Akonadi::ResourceManager(AkDBus::serviceName(AkDBus::Server),
                                                           QStringLiteral("/ResourceManager"),
                                                           QDBusConnection::sessionBus(), this));
        resmanager->addResourceInstance(agentIdentifier, type.capabilities);
    }
}

void AgentManager::addSearch(const QString &query, const QString &queryLanguage, qint64 resultCollectionId)
{
    akDebug() << "AgentManager::addSearch" << query << queryLanguage << resultCollectionId;
    Q_FOREACH (const AgentInstance::Ptr &instance, mAgentInstances) {
        const AgentType type = mAgents.value(instance->agentType());
        if (type.capabilities.contains(AgentType::CapabilitySearch) && instance->searchInterface()) {
            instance->searchInterface()->addSearch(query, queryLanguage, resultCollectionId);
        }
    }
}

void AgentManager::removeSearch(quint64 resultCollectionId)
{
    akDebug() << "AgentManager::removeSearch" << resultCollectionId;
    Q_FOREACH (const AgentInstance::Ptr &instance, mAgentInstances) {
        const AgentType type = mAgents.value(instance->agentType());
        if (type.capabilities.contains(AgentType::CapabilitySearch) && instance->searchInterface()) {
            instance->searchInterface()->removeSearch(resultCollectionId);
        }
    }
}

void AgentManager::agentServerFailure()
{
    akError() << "Failed to start AgentServer!";
    // if ( requiresAgentServer )
    //   QCoreApplication::instance()->exit( 255 );
}

void AgentManager::serverFailure()
{
    QCoreApplication::instance()->exit(255);
}
