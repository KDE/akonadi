/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "agenttype.h"
#include "agentmanager.h"

#include <private/standarddirs_p.h>
#include <private/capabilities_p.h>

#include <shared/akdebug.h>

#include <KConfigGroup>
#include <KDesktopFile>

using namespace Akonadi;

const QLatin1String AgentType::CapabilityUnique = QLatin1String(AKONADI_AGENT_CAPABILITY_UNIQUE);
const QLatin1String AgentType::CapabilityResource = QLatin1String(AKONADI_AGENT_CAPABILITY_RESOURCE);
const QLatin1String AgentType::CapabilityAutostart = QLatin1String(AKONADI_AGENT_CAPABILITY_AUTOSTART);
const QLatin1String AgentType::CapabilityPreprocessor = QLatin1String(AKONADI_AGENT_CAPABILITY_PREPROCESSOR);
const QLatin1String AgentType::CapabilitySearch = QLatin1String(AKONADI_AGENT_CAPABILITY_SEARCH);

AgentType::AgentType()
    : instanceCounter(0)
    , launchMethod(Process)
{
}

bool AgentType::load(const QString &fileName, AgentManager *manager)
{
    Q_UNUSED(manager);

    if (!KDesktopFile::isDesktopFile(fileName)) {
        return false;
    }

    KDesktopFile desktopFile(fileName);
    KConfigGroup group = desktopFile.desktopGroup();

    Q_FOREACH (const QString &key, group.keyList()) {
        if (key.startsWith(QLatin1String("X-Akonadi-Custom-"))) {
            QString customKey = key.mid(17, key.length());
            custom[customKey] = group.readEntry(key, QStringList());
        }
    }

    name = desktopFile.readName();
    comment = desktopFile.readComment();
    icon = desktopFile.readIcon();
    mimeTypes = group.readEntry(QStringLiteral("X-Akonadi-MimeTypes"), QStringList());
    capabilities = group.readEntry(QStringLiteral("X-Akonadi-Capabilities"), QStringList());
    exec = group.readEntry(QStringLiteral("Exec"));
    identifier = group.readEntry(QStringLiteral("X-Akonadi-Identifier"));
    launchMethod = Process; // Save default

    const QString method = group.readEntry(QStringLiteral("X-Akonadi-LaunchMethod"));
    if (method.compare(QLatin1String("AgentProcess"), Qt::CaseInsensitive) == 0) {
        launchMethod = Process;
    } else if (method.compare(QLatin1String("AgentServer"), Qt::CaseInsensitive) == 0) {
        launchMethod = Server;
    } else if (method.compare(QLatin1String("AgentLauncher"), Qt::CaseInsensitive) == 0) {
        launchMethod = Launcher;
    } else if (!method.isEmpty()) {
        akError() << Q_FUNC_INFO << "Invalid exec method:" << method << "falling back to AgentProcess";
    }

    if (identifier.isEmpty()) {
        akError() << Q_FUNC_INFO << "Agent desktop file" << fileName << "contains empty identifier";
        return false;
    }
    if (exec.isEmpty()) {
        akError() << Q_FUNC_INFO << "Agent desktop file" << fileName << "contains empty Exec entry";
        return false;
    }

    // autostart implies unique
    if (capabilities.contains(CapabilityAutostart) && !capabilities.contains(CapabilityUnique)) {
        capabilities << CapabilityUnique;
    }

    // load instance count if needed
    if (!capabilities.contains(CapabilityUnique)) {
        QSettings agentrc(Akonadi::StandardDirs::agentConfigFile(XdgBaseDirs::ReadOnly), QSettings::IniFormat);
        instanceCounter = agentrc.value(QStringLiteral("InstanceCounters/%1/InstanceCounter")
                                        .arg(identifier), 0).toInt();
    }

    return true;
}

void AgentType::save(QSettings *config) const
{
    Q_ASSERT(config);
    if (!capabilities.contains(CapabilityUnique)) {
        config->setValue(QStringLiteral("InstanceCounters/%1/InstanceCounter").arg(identifier), instanceCounter);
    }
}
