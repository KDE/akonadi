/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "agentconfigurationfactorybase.h"
#include "agentconfigurationwidget.h"
#include "agentinstance.h"

#include <QPluginLoader>
#include <QPointer>

#include <memory>

namespace Akonadi
{
class Q_DECL_HIDDEN AgentConfigurationWidget::AgentConfigurationWidgetPrivate
{
private:
    struct PluginLoaderDeleter {
        inline void operator()(QPluginLoader *loader)
        {
            loader->unload();
            delete loader;
        }
    };

public:
    AgentConfigurationWidgetPrivate(const AgentInstance &instance);
    ~AgentConfigurationWidgetPrivate();

    void setupErrorWidget(QWidget *parent, const QString &text);
    bool loadPlugin(const QString &pluginPath);

    std::unique_ptr<QPluginLoader, PluginLoaderDeleter> loader;
    QPointer<AgentConfigurationFactoryBase> factory = nullptr;
    QPointer<AgentConfigurationBase> plugin = nullptr;
    QWidget *baseWidget = nullptr;
    AgentInstance agentInstance;
};

}

