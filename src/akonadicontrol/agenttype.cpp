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

#include <private/xdgbasedirs_p.h>
#include <private/capabilities_p.h>

#include <shared/akdebug.h>
#include <shared/akstandarddirs.h>

#include <QSettings>

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

QString AgentType::readString(const QSettings &file, const QString &key)
{
    const QVariant value = file.value(key);
    if (value.isNull()) {
        return QString();
    } else if (value.canConvert<QString>()) {
        return QString::fromUtf8(value.toByteArray());
    } else if (value.canConvert<QStringList>()) {
        // This is a workaround for QSettings interpreting value with a comma as
        // a QStringList, which is not compatible with KConfig. KConfig reads everything
        // as a QByteArray and splits it to a list when requested. See BKO#330010
        // TODO KF5: If we end up in Tier 2 or above, depend on KConfig for parsing
        // .desktop files
        const QStringList parts = value.toStringList();
        QStringList utf8Parts;
        utf8Parts.reserve(parts.size());
        Q_FOREACH (const QString &part, parts) {
            utf8Parts << QString::fromUtf8(part.toLatin1());
        }
        return utf8Parts.join(QStringLiteral(", "));
    } else {
        akError() << "Agent desktop file" << file.fileName() << "contains invalid value for key" << key;
        return QString();
    }
}

bool AgentType::load(const QString &fileName, AgentManager *manager)
{
    Q_UNUSED(manager);

    QSettings file(fileName, QSettings::IniFormat);
    file.beginGroup(QStringLiteral("Desktop Entry"));

    Q_FOREACH (const QString &key, file.allKeys()) {
        if (key.startsWith(QLatin1String("Name["))) {
            QString lang = key.mid(5, key.length() - 6);
            name.insert(lang, readString(file, key));
        } else if (key == QLatin1String("Name")) {
            name.insert(QStringLiteral("en_US"), readString(file, key));
        } else if (key.startsWith(QLatin1String("Comment["))) {
            QString lang = key.mid(8, key.length() - 9);
            comment.insert(lang, readString(file, key));
        } else if (key == QLatin1String("Comment")) {
            comment.insert(QStringLiteral("en_US"), readString(file, key));
        } else if (key.startsWith(QLatin1String("X-Akonadi-Custom-"))) {
            QString customKey = key.mid(17, key.length());
            custom[customKey] = file.value(key);
        }
    }
    icon = file.value(QStringLiteral("Icon")).toString();
    mimeTypes = file.value(QStringLiteral("X-Akonadi-MimeTypes")).toStringList();
    capabilities = file.value(QStringLiteral("X-Akonadi-Capabilities")).toStringList();
    exec = file.value(QStringLiteral("Exec")).toString();
    identifier = file.value(QStringLiteral("X-Akonadi-Identifier")).toString();
    launchMethod = Process; // Save default

    const QString method = file.value(QStringLiteral("X-Akonadi-LaunchMethod")).toString();
    if (method.compare(QLatin1String("AgentProcess"), Qt::CaseInsensitive) == 0) {
        launchMethod = Process;
    } else if (method.compare(QLatin1String("AgentServer"), Qt::CaseInsensitive) == 0) {
        launchMethod = Server;
    } else if (method.compare(QLatin1String("AgentLauncher"), Qt::CaseInsensitive) == 0) {
        launchMethod = Launcher;
    } else if (!method.isEmpty()) {
        akError() << Q_FUNC_INFO << "Invalid exec method:" << method << "falling back to AgentProcess";
    }

    file.endGroup();

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
        QSettings agentrc(AkStandardDirs::agentConfigFile(XdgBaseDirs::ReadOnly), QSettings::IniFormat);
        instanceCounter = agentrc.value(QString::fromLatin1("InstanceCounters/%1/InstanceCounter")
                                        .arg(identifier), 0).toInt();
    }

    return true;
}

void AgentType::save(QSettings *config) const
{
    Q_ASSERT(config);
    if (!capabilities.contains(CapabilityUnique)) {
        config->setValue(QString::fromLatin1("InstanceCounters/%1/InstanceCounter").arg(identifier), instanceCounter);
    }
}
