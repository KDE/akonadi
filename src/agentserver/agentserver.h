/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_AGENTSERVER_H
#define AKONADI_AGENTSERVER_H

#include "agentpluginloader.h"

#include <QHash>
#include <QObject>
#include <QQueue>

namespace Akonadi {

class AgentThread;

class AgentServer : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Akonadi.AgentServer")

    typedef QPair<QString, qlonglong> ConfigureInfo;

public:
    explicit AgentServer(QObject *parent = Q_NULLPTR);
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
    bool m_processingConfigureRequests;
    bool m_quiting;
};

}

#endif
