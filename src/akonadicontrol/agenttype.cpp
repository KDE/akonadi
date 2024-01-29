/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agenttype.h"
#include "agentmanager.h"
#include "akonadicontrol_debug.h"

#include <private/capabilities_p.h>
#include <private/standarddirs_p.h>

#include <KConfigGroup>
#include <KDesktopFile>

using namespace Akonadi;

const QLatin1StringView AgentType::CapabilityUnique = QLatin1String(AKONADI_AGENT_CAPABILITY_UNIQUE);
const QLatin1StringView AgentType::CapabilityResource = QLatin1String(AKONADI_AGENT_CAPABILITY_RESOURCE);
const QLatin1StringView AgentType::CapabilityAutostart = QLatin1String(AKONADI_AGENT_CAPABILITY_AUTOSTART);
const QLatin1StringView AgentType::CapabilityPreprocessor = QLatin1String(AKONADI_AGENT_CAPABILITY_PREPROCESSOR);
const QLatin1StringView AgentType::CapabilitySearch = QLatin1String(AKONADI_AGENT_CAPABILITY_SEARCH);

AgentType::AgentType()
{
}

bool AgentType::load(const QString &fileName, AgentManager *manager)
{
    Q_UNUSED(manager)

    if (!KDesktopFile::isDesktopFile(fileName)) {
        return false;
    }

    KDesktopFile desktopFile(fileName);
    KConfigGroup group = desktopFile.desktopGroup();

    const QStringList keyList(group.keyList());
    for (const QString &key : keyList) {
        if (key.startsWith(QLatin1StringView("X-Akonadi-Custom-"))) {
            const QString customKey = key.mid(17, key.length());
            const QStringList val = group.readEntry(key, QStringList());
            if (val.size() == 1) {
                custom[customKey] = QVariant(val[0]);
            } else {
                custom[customKey] = val;
            }
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
    if (method.compare(QLatin1StringView("AgentProcess"), Qt::CaseInsensitive) == 0) {
        launchMethod = Process;
    } else if (method.compare(QLatin1StringView("AgentServer"), Qt::CaseInsensitive) == 0) {
        launchMethod = Server;
    } else if (method.compare(QLatin1StringView("AgentLauncher"), Qt::CaseInsensitive) == 0) {
        launchMethod = Launcher;
    } else if (!method.isEmpty()) {
        qCWarning(AKONADICONTROL_LOG) << "Invalid exec method:" << method << "falling back to AgentProcess";
    }

    if (identifier.isEmpty()) {
        qCWarning(AKONADICONTROL_LOG) << "Agent desktop file" << fileName << "contains empty identifier";
        return false;
    }
    if (exec.isEmpty()) {
        qCWarning(AKONADICONTROL_LOG) << "Agent desktop file" << fileName << "contains empty Exec entry";
        return false;
    }

    // autostart implies unique
    if (capabilities.contains(CapabilityAutostart) && !capabilities.contains(CapabilityUnique)) {
        capabilities << CapabilityUnique;
    }

    // load instance count if needed
    if (!capabilities.contains(CapabilityUnique)) {
        QSettings agentrc(StandardDirs::agentsConfigFile(StandardDirs::ReadOnly), QSettings::IniFormat);
        instanceCounter = agentrc.value(QStringLiteral("InstanceCounters/%1/InstanceCounter").arg(identifier), 0).toInt();
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
