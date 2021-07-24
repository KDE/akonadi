/*
    SPDX-FileCopyrightText: 2010 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentpluginloader.h"
#include "akonadiagentserver_debug.h"

#include <QApplication>
#include <QPluginLoader>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    if (app.arguments().size() != 3) { // Expected usage: ./agent_launcher ${plugin_name} ${identifier}
        qCDebug(AKONADIAGENTSERVER_LOG) << "Invalid usage: expected: ./agent_launcher pluginName agentIdentifier";
        return 1;
    }

    const QString agentPluginName = app.arguments().at(1);
    const QString agentIdentifier = app.arguments().at(2);

    AgentPluginLoader loader;
    QPluginLoader *factory = loader.load(agentPluginName);
    if (!factory) {
        return 1;
    }

    QObject *instance = nullptr;
    // clang-format off
    const bool invokeSucceeded = QMetaObject::invokeMethod(factory->instance(),
                                                           "createInstance",
                                                           Qt::DirectConnection,
                                                           Q_RETURN_ARG(QObject *, instance),
                                                           Q_ARG(QString, agentIdentifier));
    // clang-format on
    if (invokeSucceeded) {
        qCDebug(AKONADIAGENTSERVER_LOG) << "Agent instance created in separate process.";
    } else {
        qCDebug(AKONADIAGENTSERVER_LOG) << "Agent instance creation in separate process failed";
        return 2;
    }

    const int rv = app.exec();
    delete instance;
    return rv;
}
