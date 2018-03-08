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
#include "akonadi-prefix.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QFile>
#include <QFileInfo>
#include <QVector>
#include <QDir>

using namespace Akonadi;

namespace {

QString buildFullRelPath(const char *resource, const QString &relPath)
{
    QString fullRelPath = QStringLiteral("/akonadi");
#ifdef Q_OS_WIN
    // On Windows all Generic*Location fall into ~/AppData/Local so we need to disambiguate
    // inside the "akonadi" folder whether it's data or config.
    fullRelPath += QLatin1Char('/') + QString::fromLocal8Bit(resource);
#else
    Q_UNUSED(resource);
#endif

    if (Akonadi::Instance::hasIdentifier()) {
        fullRelPath += QStringLiteral("/instance/") + Akonadi::Instance::identifier();
    }
    if (!relPath.isEmpty()) {
        fullRelPath += QLatin1Char('/') + relPath;
    }
    return fullRelPath;
}

}

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
    const QString fullRelPath = buildFullRelPath(resource, relPath);
    if (qstrncmp(resource, "config", 6) == 0) {
        return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + fullRelPath;
    } else if (qstrncmp(resource, "data", 4) == 0) {
        return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + fullRelPath;
    } else {
        qt_assert_x(__FUNCTION__, "Invalid resource type", __FILE__, __LINE__);
        return {};
    }
}

QString StandardDirs::locateResourceFile(const char *resource, const QString &relPath)
{
    const QString fullRelPath = buildFullRelPath(resource, relPath);
    QVector<QStandardPaths::StandardLocation> userLocations;
    QStandardPaths::StandardLocation genericLocation;
    QString fallback;
    if (qstrncmp(resource, "config", 6) == 0) {
        userLocations = { QStandardPaths::AppConfigLocation,
                          QStandardPaths::ConfigLocation };
        genericLocation = QStandardPaths::GenericConfigLocation;
        fallback = QDir::toNativeSeparators(QStringLiteral(AKONADIPREFIX "/" AKONADICONFIG));
    } else if (qstrncmp(resource, "data", 4) == 0) {
        userLocations = { QStandardPaths::AppLocalDataLocation,
                          QStandardPaths::AppDataLocation };
        genericLocation = QStandardPaths::GenericDataLocation;
        fallback = QDir::toNativeSeparators(QStringLiteral(AKONADIPREFIX "/" AKONADIDATA));
    } else {
        qt_assert_x(__FUNCTION__, "Invalid resource type", __FILE__, __LINE__);
        return {};
    }

    const auto locateFile = [](QStandardPaths::StandardLocation location, const QString &relPath) -> QString {
        const auto path = QStandardPaths::locate(location, relPath);
        if (!path.isEmpty()) {
            QFileInfo file(path);
            if (file.exists() && file.isFile() && file.isReadable()) {
                return path;
            }
        }
        return {};
    };

    // Always honor instance in user-specific locations
    for (const auto location : qAsConst(userLocations)) {
        const auto path = locateFile(location, fullRelPath);
        if (!path.isEmpty()) {
            return path;
        }
    }

    // First try instance-specific path in generic locations
    auto path = locateFile(genericLocation, fullRelPath);
    if (!path.isEmpty()) {
        return path;
    }

    // Fallback to global instance path in generic locations
    path = locateFile(genericLocation, QStringLiteral("/akonadi/") + relPath);
    if (!path.isEmpty()) {
        return path;
    }

    QFile f(fallback + QStringLiteral("/akonadi/") + relPath);
    if (f.exists()) {
        return f.fileName();
    }

    return {};
}

QStringList StandardDirs::locateAllResourceDirs(const QString &relPath)
{
    auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, relPath,
                                          QStandardPaths::LocateDirectory);

    const auto fallback = QDir::toNativeSeparators(QStringLiteral(AKONADIPREFIX "/" AKONADIDATA "/") + relPath);
    if (!dirs.contains(fallback)) {
        if (QDir::root().exists(fallback)) {
            dirs.push_back(fallback);
        }
    }
    return dirs;
}

QString StandardDirs::findExecutable(const QString &executableName)
{
    QString executable = QStandardPaths::findExecutable(executableName, {qApp->applicationDirPath()});
    if (executable.isEmpty()) {
        executable = QStandardPaths::findExecutable(executableName);
    }
    return executable;
}
