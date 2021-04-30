/*
    SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QHash>
class QPluginLoader;

class AgentPluginLoader
{
public:
    AgentPluginLoader();

    /**
      Deletes all instantiated QPluginLoaders.
     */
    ~AgentPluginLoader();

    /**
      Returns the loader for plugins with @p pluginName. Callers must not
      take ownership over the returned loader. Loaders will be unloaded and deleted
      when the AgentPluginLoader goes out of scope/gets deleted.

      @return the plugin for @p pluginName or 0 if the plugin is not found.
     */
    Q_REQUIRED_RESULT QPluginLoader *load(const QString &pluginName);

private:
    Q_DISABLE_COPY(AgentPluginLoader)
    QHash<QString, QPluginLoader *> m_pluginLoaders;
};

