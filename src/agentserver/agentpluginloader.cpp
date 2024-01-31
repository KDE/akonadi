/*
    SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#include "agentpluginloader.h"
#include "akonadiagentserver_debug.h"

#include "shared/akdebug.h"

#include <QPluginLoader>

AgentPluginLoader::AgentPluginLoader()
{
}

AgentPluginLoader::~AgentPluginLoader()
{
    qDeleteAll(m_pluginLoaders);
    m_pluginLoaders.clear();
}

QPluginLoader *AgentPluginLoader::load(const QString &pluginName)
{
    QPluginLoader *loader = m_pluginLoaders.value(pluginName);
    if (loader) {
        return loader;
    } else {
        loader = new QPluginLoader(pluginName);
        if (!loader->load()) {
            qCWarning(AKONADIAGENTSERVER_LOG) << "Failed to load agent: " << loader->errorString();
            delete loader;
            return nullptr;
        }
        m_pluginLoaders.insert(pluginName, loader);
        return loader;
    }
}
