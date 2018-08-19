/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_AGENTCONFIGURATIONWIDGET_P_H
#define AKONADI_AGENTCONFIGURATIONWIDGET_P_H

#include "agentconfigurationwidget.h"
#include "agentconfigurationfactorybase.h"
#include "agentinstance.h"

#include <QPluginLoader>
#include <QPointer>

#include <memory>

namespace Akonadi {


class Q_DECL_HIDDEN AgentConfigurationWidget::Private {
private:
    struct PluginLoaderDeleter {
        inline void operator()(QPluginLoader *loader) {
            loader->unload();
            delete loader;
        }
    };
public:
    Private(const AgentInstance &instance);
    ~Private();

    void setupErrorWidget(QWidget *parent, const QString &text);
    bool loadPlugin(const QString &pluginPath);

    std::unique_ptr<QPluginLoader, PluginLoaderDeleter> loader;
    QPointer<AgentConfigurationFactoryBase> factory = nullptr;
    QPointer<AgentConfigurationBase> plugin = nullptr;
    QWidget *baseWidget = nullptr;
    AgentInstance agentInstance;
};

}


#endif // AKONADI_AGENTCONFIGURATIONWIDGET_P_H
