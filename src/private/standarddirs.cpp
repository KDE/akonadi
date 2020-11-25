/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "standarddirs_p.h"
#include "instance_p.h"
#include "akonadiprivate_debug.h"

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
    Q_UNUSED(resource)
#endif

    if (Akonadi::Instance::hasIdentifier()) {
        fullRelPath += QLatin1String("/instance/") + Akonadi::Instance::identifier();
    }
    if (!relPath.isEmpty()) {
        fullRelPath += QLatin1Char('/') + relPath;
    }
    return fullRelPath;
}

} // namespace

QString StandardDirs::configFile(const QString &configFile, FileAccessMode openMode)
{
    const QString savePath = StandardDirs::saveDir("config") + QLatin1Char('/') + configFile;
    if (openMode == WriteOnly) {
        return savePath;
    }


    auto path = QStandardPaths::locate(QStandardPaths::GenericConfigLocation, QLatin1String("akonadi/") + configFile);
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
    QString fullPath;
    if (qstrncmp(resource, "config", 6) == 0) {
        fullPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + fullRelPath;
    } else if (qstrncmp(resource, "data", 4) == 0) {
        fullPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + fullRelPath;
    } else if (qstrncmp(resource, "runtime", 7) == 0) {
        fullPath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation) + fullRelPath;
    } else {
        qt_assert_x(__FUNCTION__, "Invalid resource type", __FILE__, __LINE__);
        return {};
    }

    // ensure directory exists or is created
    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists()) {
        if (fileInfo.isDir()) {
            return fullPath;
        } else {
            qCWarning(AKONADIPRIVATE_LOG) << "StandardDirs::saveDir: '" << fileInfo.absoluteFilePath()
                                          << "' exists but is not a directory";
        }
    } else {
        if (!QDir::home().mkpath(fileInfo.absoluteFilePath())) {
            qCWarning(AKONADIPRIVATE_LOG) << "StandardDirs::saveDir: failed to create directory '"
                                          << fileInfo.absoluteFilePath() << "'";
        } else {
            return fullPath;
        }
    }

    return {};
}

QString StandardDirs::locateResourceFile(const char *resource, const QString &relPath)
{
    const QString fullRelPath = buildFullRelPath(resource, relPath);
    QVector<QStandardPaths::StandardLocation> userLocations;
    QStandardPaths::StandardLocation genericLocation;
    if (qstrncmp(resource, "config", 6) == 0) {
        userLocations = { QStandardPaths::AppConfigLocation,
                          QStandardPaths::ConfigLocation };
        genericLocation = QStandardPaths::GenericConfigLocation;
    } else if (qstrncmp(resource, "data", 4) == 0) {
        userLocations = { QStandardPaths::AppLocalDataLocation,
                          QStandardPaths::AppDataLocation };
        genericLocation = QStandardPaths::GenericDataLocation;
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
    path = locateFile(genericLocation, QLatin1String("/akonadi/") + relPath);
    if (!path.isEmpty()) {
        return path;
    }

    return {};
}

QStringList StandardDirs::locateAllResourceDirs(const QString &relPath)
{
    return QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, relPath,
                                     QStandardPaths::LocateDirectory);
}

QString StandardDirs::findExecutable(const QString &executableName)
{
    QString executable = QStandardPaths::findExecutable(executableName, {qApp->applicationDirPath()});
    if (executable.isEmpty()) {
        executable = QStandardPaths::findExecutable(executableName);
    }
    return executable;
}
