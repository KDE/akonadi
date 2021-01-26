/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentserver.h"
#include "agentthread.h"
#include "akonadiagentserver_debug.h"

#include <private/dbus_p.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QPluginLoader>
#include <QTimer>

using namespace Akonadi;

AgentServer::AgentServer(QObject *parent)
    : QObject(parent)
{
    QDBusConnection::sessionBus().registerObject(QStringLiteral(AKONADI_DBUS_AGENTSERVER_PATH), this, QDBusConnection::ExportScriptableSlots);
}

AgentServer::~AgentServer()
{
    qCDebug(AKONADIAGENTSERVER_LOG) << Q_FUNC_INFO;
    if (!m_quiting) {
        quit();
    }
}

void AgentServer::agentInstanceConfigure(const QString &identifier, qlonglong windowId)
{
    m_configureQueue.enqueue(ConfigureInfo(identifier, windowId));
    if (!m_processingConfigureRequests) { // Start processing the requests if needed.
        QTimer::singleShot(0, this, &AgentServer::processConfigureRequest);
    }
}

bool AgentServer::started(const QString &identifier) const
{
    return m_agents.contains(identifier);
}

void AgentServer::startAgent(const QString &identifier, const QString &typeIdentifier, const QString &fileName)
{
    qCDebug(AKONADIAGENTSERVER_LOG) << "Starting agent" << identifier << "of type" << typeIdentifier << "(file:" << fileName << ")";

    // First try to load it staticly
    const QObjectList objList = QPluginLoader::staticInstances();
    for (QObject *plugin : objList) {
        if (plugin->objectName() == fileName) {
            auto *thread = new AgentThread(identifier, plugin, this);
            m_agents.insert(identifier, thread);
            thread->start();
            return;
        }
    }

    QPluginLoader *loader = m_agentLoader.load(fileName);
    if (!loader) {
        return; // No plugin found, debug output in AgentLoader.
    }

    Q_ASSERT(loader->isLoaded());

    auto *thread = new AgentThread(identifier, loader->instance(), this);
    m_agents.insert(identifier, thread);
    thread->start();
}

void AgentServer::stopAgent(const QString &identifier)
{
    AgentThread *thread = m_agents.take(identifier);
    if (thread) {
        thread->quit();
        thread->wait();
        delete thread;
    }
}

void AgentServer::quit()
{
    Q_ASSERT(!m_quiting);
    m_quiting = true;

    QMutableHashIterator<QString, AgentThread *> it(m_agents);
    while (it.hasNext()) {
        stopAgent(it.next().key());
    }

    QCoreApplication::instance()->quit();
}

void AgentServer::processConfigureRequest()
{
    if (m_processingConfigureRequests) {
        return; // Protect against reentrancy
    }

    m_processingConfigureRequests = true;

    while (!m_configureQueue.empty()) {
        const ConfigureInfo info = m_configureQueue.dequeue();
        // call configure on the agent with id info.first for windowId info.second.
        Q_ASSERT(m_agents.contains(info.first));
        AgentThread *thread = m_agents.value(info.first);
        thread->configure(info.second);
    }

    m_processingConfigureRequests = false;
}
