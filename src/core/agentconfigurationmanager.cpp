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

#include "agentconfigurationmanager_p.h"
#include "akonadicore_debug.h"
#include "servermanager.h"

#include <QDBusConnectionInterface>
#include <QCoreApplication>
#include <QPluginLoader>
#include <QDir>
#include <QFileInfo>

#include <KDBusConnectionPool>

namespace Akonadi {
class Q_DECL_HIDDEN AgentConfigurationManager::Private {
public:
    QString serviceName(const QString &instance) const
    {
        QString service = QStringLiteral("org.freedesktop.Akonadi.AgentConfig.%1").arg(instance);
        if (ServerManager::self()->hasInstanceIdentifier()) {
            service += QLatin1Char('.') + ServerManager::self()->instanceIdentifier();
        }
        return service;
    }
};
}

using namespace Akonadi;

AgentConfigurationManager *AgentConfigurationManager::sInstance = nullptr;

AgentConfigurationManager::AgentConfigurationManager(QObject *parent)
    : QObject(parent)
{
}

AgentConfigurationManager *AgentConfigurationManager::self()
{
    if (sInstance == nullptr) {
        sInstance = new AgentConfigurationManager();
    }
    return sInstance;
}

AgentConfigurationManager::~AgentConfigurationManager()
{
}

bool AgentConfigurationManager::registerInstanceConfiguration(const QString &instance)
{
    const auto serviceName = d->serviceName(instance);
    QDBusConnection conn = KDBusConnectionPool::threadConnection();
    if (conn.interface()->isServiceRegistered(serviceName)) {
        qCDebug(AKONADICORE_LOG) << "Service " << serviceName << " is already registered";
        return false;
    }

    return conn.registerService(serviceName);
}

void AgentConfigurationManager::unregisterInstanceConfiguration(const QString &instance)
{
    const auto serviceName = d->serviceName(instance);
    KDBusConnectionPool::threadConnection().unregisterService(serviceName);
}

bool AgentConfigurationManager::isInstanceRegistered(const QString &instance) const
{
    const auto serviceName = d->serviceName(instance);
    return KDBusConnectionPool::threadConnection().interface()->isServiceRegistered(serviceName);
}

QString AgentConfigurationManager::findConfigPlugin(const QString &type) const
{
    const auto libPaths = QCoreApplication::libraryPaths();
    for (const auto &libPath : libPaths) {
        const QString pluginPath = QStringLiteral("%1/akonadi/config/").arg(libPath);
        const auto libs = QDir(pluginPath).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
        for (const auto &lib : libs) {
            QPluginLoader loader(lib.absoluteFilePath());
            const auto metaData = loader.metaData().toVariantMap();
            if (metaData.value(QStringLiteral("IID")).toString() != QLatin1String("org.freedesktop.Akonadi.AgentConfig")) {
                continue;
            }
            const auto md = metaData.value(QStringLiteral("MetaData")).toMap();
            if (md.value(QStringLiteral("X-Akonadi-PluginType")).toString() != QLatin1String("AgentConfig")) {
                continue;
            }
            if (!type.startsWith(md.value(QStringLiteral("X-Akonadi-AgentConfig-Type")).toString())) {
                continue;
            }
            return loader.fileName();
        }
    }

    return {};
}
