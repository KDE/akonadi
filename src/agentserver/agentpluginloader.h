/*
    Copyright (c) 2010 Bertjan Broeksema <broeksema@kde.org>

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
#ifndef AGENTPLUGINLOADER_H
#define AGENTPLUGINLOADER_H

#include <QtCore/QHash>
#include <QtCore/QPluginLoader>

class AgentPluginLoader
{
public:
    AgentPluginLoader();

    /**
      Deletes all instantiated QPluginLoaders.
     */
    ~AgentPluginLoader();

    /**
      Returns the loader for plugins with @param pluginName. Callers must not
      take ownership over the returned loader. Loaders will be unloaded and deleted
      when the AgentPluginLoader goes out of scope/gets deleted.

      @return the plugin for @param pluginName or 0 if the plugin is not found.
     */
    QPluginLoader *load(const QString &pluginName);

private:
    Q_DISABLE_COPY(AgentPluginLoader)
    QHash<QString, QPluginLoader *> m_pluginLoaders;
};

#endif // AGENTPLUGINLOADER_H
