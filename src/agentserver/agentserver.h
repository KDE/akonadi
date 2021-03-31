/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentpluginloader.h"

#include <QHash>
#include <QObject>
#include <QQueue>

namespace Akonadi
{
class AgentThread;

class AgentServer : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.AgentServer")

    typedef QPair<QString, qlonglong> ConfigureInfo;

public:
    explicit AgentServer(QObject *parent = nullptr);
    ~AgentServer();

public Q_SLOTS:
    Q_SCRIPTABLE void agentInstanceConfigure(const QString &identifier, qlonglong windowId);
    Q_SCRIPTABLE bool started(const QString &identifier) const;
    Q_SCRIPTABLE void startAgent(const QString &identifier, const QString &typeIdentifier, const QString &fileName);
    Q_SCRIPTABLE void stopAgent(const QString &identifier);
    Q_SCRIPTABLE void quit();

private Q_SLOTS:
    void processConfigureRequest();

private:
    QHash<QString, AgentThread *> m_agents;
    QQueue<ConfigureInfo> m_configureQueue;
    AgentPluginLoader m_agentLoader;
    bool m_processingConfigureRequests = false;
    bool m_quiting = false;
};

}

