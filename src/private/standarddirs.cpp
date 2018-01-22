/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>
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

#include "standarddirs_p.h"
#include "instance_p.h"

#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QVector>

using namespace Akonadi;

QString StandardDirs::configFile(const QString &configFile, FileAccessMode openMode)
{
    const QString savePath = StandardDirs::saveDir("config") + QLatin1Char('/') + configFile;
    if (openMode == WriteOnly) {
        return savePath;
    }


    auto path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QStringLiteral("akonadi/") + configFile);
    // HACK: when using instance namespaces, ignore the non-namespaced file
    if (Akonadi::Instance::hasIdentifier() && path.startsWith(QStandardPaths::writableLocation(QStandardPaths::ConfigLocation))) {
        path.clear();
    }

    if (path.isEmpty()) {
        return savePath;
    } else if (openMode == ReadOnly || path == savePath) {
        return path;
    }

    // file found in system paths and mode is ReadWrite, thus
    // we copy to the home path location and return this path
    QFile::copy(path, savePath);
    return savePath;
}

QString StandardDirs::serverConfigFile(FileAccessMode openMode)
{
    return configFile(QStringLiteral("akonadiserverrc"), openMode);
}

QString StandardDirs::connectionConfigFile(FileAccessMode openMode)
{
    return configFile(QStringLiteral("akonadiconnectionrc"), openMode);
}

QString StandardDirs::agentsConfigFile(FileAccessMode openMode)
{
    return configFile(QStringLiteral("agentsrc"), openMode);
}

QString StandardDirs::agentConfigFile(const QString &identifier, FileAccessMode openMode)
{
    return configFile(QStringLiteral("agent_config_") + identifier, openMode);
}

QString StandardDirs::saveDir(const char *resource, const QString &relPath)
{
    QString fullRelPath = QStringLiteral("/akonadi");
    if (Akonadi::Instance::hasIdentifier()) {
        fullRelPath += QStringLiteral("/instance/") + Akonadi::Instance::identifier();
    }
    if (!relPath.isEmpty()) {
        fullRelPath += QLatin1Char('/') + relPath;
    }
    if (qstrncmp(resource, "config", 6) == 0) {
        return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + fullRelPath;
    } else if (qstrncmp(resource, "data", 4) == 0) {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + fullRelPath;
    } else {
        qt_assert_x(__FUNCTION__, "Invalid resource type", __FILE__, __LINE__);
    }

    return {};
}
