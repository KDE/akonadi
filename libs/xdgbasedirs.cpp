/***************************************************************************
 *   Copyright (C) 2007 by Kevin Krammer <kevin.krammer@gmx.at>            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "xdgbasedirs_p.h"

#include "akonadi-prefix.h" // for prefix defines

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QSettings>

#include <cstdlib>

static QStringList alternateExecPaths(const QString &path)
{
    QStringList pathList;

    pathList << path;

#if defined(Q_OS_WIN) //krazy:exclude=cpp
    pathList << path + QLatin1String(".exe");
#elif defined(Q_OS_MAC) //krazy:exclude=cpp
    pathList << path + QLatin1String(".app/Contents/MacOS/") + path.section(QLatin1Char('/'), -1);
#endif

    return pathList;
}

static QStringList splitPathList(const QString &pathList)
{
#if defined(Q_OS_WIN) //krazy:exclude=cpp
    return pathList.split(QLatin1Char(';'));
#else
    return pathList.split(QLatin1Char(':'));
#endif
}

namespace Akonadi {

class XdgBaseDirsPrivate
{
public:
    XdgBaseDirsPrivate()
    {
    }

    ~XdgBaseDirsPrivate()
    {
    }
};

class XdgBaseDirsSingleton
{
public:
    QString homePath(const char *variable, const char *defaultSubDir);

    QStringList systemPathList(const char *variable, const char *defaultDirList);

public:
    QString mConfigHome;
    QString mDataHome;

    QStringList mConfigDirs;
    QStringList mDataDirs;
    QStringList mExecutableDirs;
    QStringList mPluginDirs;
};

Q_GLOBAL_STATIC(XdgBaseDirsSingleton, instance)

}

using namespace Akonadi;

XdgBaseDirs::XdgBaseDirs()
    : d(new XdgBaseDirsPrivate())
{
}

XdgBaseDirs::~XdgBaseDirs()
{
    delete d;
}

QString XdgBaseDirs::homePath(const char *resource)
{
    if (qstrncmp("data", resource, 4) == 0) {
        if (instance()->mDataHome.isEmpty()) {
            instance()->mDataHome = instance()->homePath("XDG_DATA_HOME", ".local/share");
        }
        return instance()->mDataHome;
    } else if (qstrncmp("config", resource, 6) == 0) {
        if (instance()->mConfigHome.isEmpty()) {
            instance()->mConfigHome = instance()->homePath("XDG_CONFIG_HOME", ".config");
        }
        return instance()->mConfigHome;
    }

    return QString();
}

QStringList XdgBaseDirs::systemPathList(const char *resource)
{
    if (qstrncmp("data", resource, 4) == 0) {
        if (instance()->mDataDirs.isEmpty()) {
#ifdef Q_OS_WIN
            QDir dir(QCoreApplication::applicationDirPath());
            dir.cdUp();
            const QString defaultPathList = dir.absoluteFilePath(QLatin1String("share"));
            QStringList dataDirs = instance()->systemPathList("XDG_DATA_DIRS", defaultPathList.toLocal8Bit().constData());
#else
            QStringList dataDirs = instance()->systemPathList("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
#endif

#ifdef Q_OS_WIN
            const QString prefixDataDir = QLatin1String(AKONADIPREFIX "/" AKONADIDATA);
#else
            const QString prefixDataDir = QLatin1String(AKONADIDATA);
#endif
            if (!dataDirs.contains(prefixDataDir)) {
                dataDirs << prefixDataDir;
            }

#if QT_VERSION < 0x050000
            // fallback for users with KDE in a different prefix and not correctly set up XDG_DATA_DIRS, hi David ;-)
            QProcess proc;
            // ### should probably rather be --path xdg-something
            const QStringList args = QStringList() << QLatin1String("--prefix");
            proc.start(QLatin1String("kde4-config"), args);
            if (proc.waitForStarted() && proc.waitForFinished() && proc.exitCode() == 0) {
                proc.setReadChannel(QProcess::StandardOutput);
                Q_FOREACH (const QString &basePath, splitPathList(QString::fromLocal8Bit(proc.readLine().trimmed()))) {
                    const QString path = basePath + QDir::separator() + QLatin1String("share");
                    if (!dataDirs.contains(path)) {
                        dataDirs << path;
                    }
                }
            }
#endif

            instance()->mDataDirs = dataDirs;
        }
#ifdef Q_OS_WIN
        QStringList dataDirs = instance()->mDataDirs;
        // on Windows installation might be scattered across several directories
        // so check if any installer providing agents has registered its base path
        QSettings agentProviders(QSettings::SystemScope, QLatin1String("Akonadi"), QLatin1String("Akonadi"));
        agentProviders.beginGroup(QLatin1String("AgentProviders"));
        Q_FOREACH (const QString &agentProvider, agentProviders.childKeys()) {
            const QString basePath = agentProviders.value(agentProvider).toString();
            if (!basePath.isEmpty()) {
                const QString path = basePath + QDir::separator() + QLatin1String("share");
                if (!dataDirs.contains(path)) {
                    dataDirs << path;
                }
            }
        }

        return dataDirs;
#else
        return instance()->mDataDirs;
#endif
    } else if (qstrncmp("config", resource, 6) == 0) {
        if (instance()->mConfigDirs.isEmpty()) {
#ifdef Q_OS_WIN
            QDir dir(QCoreApplication::applicationDirPath());
            dir.cdUp();
            const QString defaultPathList = dir.absoluteFilePath(QLatin1String("etc")) + QLatin1Char(';') + dir.absoluteFilePath(QLatin1String("share/config"));
            QStringList configDirs = instance()->systemPathList("XDG_CONFIG_DIRS", defaultPathList.toLocal8Bit().constData());
#else
            QStringList configDirs = instance()->systemPathList("XDG_CONFIG_DIRS", "/etc/xdg");
#endif

#ifdef Q_OS_WIN
            const QString prefixConfigDir = QLatin1String(AKONADIPREFIX "/" AKONADICONFIG);
#else
            const QString prefixConfigDir = QLatin1String(AKONADICONFIG);
#endif
            if (!configDirs.contains(prefixConfigDir)) {
                configDirs << prefixConfigDir;
            }

            instance()->mConfigDirs = configDirs;
        }
        return instance()->mConfigDirs;
    }

    return QStringList();
}

QString XdgBaseDirs::findResourceFile(const char *resource, const QString &relPath)
{
    const QString fullPath = homePath(resource) + QLatin1Char('/') + relPath;

    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()) {
        return fullPath;
    }

    const QStringList pathList = systemPathList(resource);

    Q_FOREACH (const QString &path, pathList) {
        fileInfo = QFileInfo(path + QLatin1Char('/') + relPath);
        if (fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable()) {
            return fileInfo.absoluteFilePath();
        }
    }

    return QString();
}

QString XdgBaseDirs::findExecutableFile(const QString &relPath, const QStringList &searchPath)
{
    if (instance()->mExecutableDirs.isEmpty()) {
        QStringList executableDirs = instance()->systemPathList("PATH", "/usr/local/bin:/usr/bin");

        const QString prefixExecutableDir = QLatin1String(AKONADIPREFIX "/bin");
        if (!executableDirs.contains(prefixExecutableDir)) {
            executableDirs << prefixExecutableDir;
        }

        if (QCoreApplication::instance() != 0) {
            const QString appExecutableDir = QCoreApplication::instance()->applicationDirPath();
            if (!executableDirs.contains(appExecutableDir)) {
                executableDirs << appExecutableDir;
            }
        }

        executableDirs += searchPath;

#if defined(Q_OS_MAC) //krazy:exclude=cpp
        executableDirs += QLatin1String(AKONADIBUNDLEPATH);
#endif
        qWarning() << "search paths: " << executableDirs;

        instance()->mExecutableDirs = executableDirs;
    }

#ifdef Q_OS_WIN
    QStringList executableDirs = instance()->mExecutableDirs;
    // on Windows installation might be scattered across several directories
    // so check if any installer providing agents has registered its base path
    QSettings agentProviders(QSettings::SystemScope, QLatin1String("Akonadi"), QLatin1String("Akonadi"));
    agentProviders.beginGroup(QLatin1String("AgentProviders"));
    Q_FOREACH (const QString &agentProvider, agentProviders.childKeys()) {
        const QString basePath = agentProviders.value(agentProvider).toString();
        if (!basePath.isEmpty()) {
            const QString path = basePath + QDir::separator() + QLatin1String("bin");
            if (!executableDirs.contains(path)) {
                executableDirs << path;
            }
        }
    }

    QStringList::const_iterator pathIt = executableDirs.constBegin();
    const QStringList::const_iterator pathEndIt = executableDirs.constEnd();
#else
    QStringList::const_iterator pathIt = instance()->mExecutableDirs.constBegin();
    const QStringList::const_iterator pathEndIt = instance()->mExecutableDirs.constEnd();
#endif
    for (; pathIt != pathEndIt; ++pathIt) {
        const QStringList fullPathList = alternateExecPaths(*pathIt + QLatin1Char('/') + relPath);

        QStringList::const_iterator it = fullPathList.constBegin();
        const QStringList::const_iterator endIt = fullPathList.constEnd();
        for (; it != endIt; ++it) {
            const QFileInfo fileInfo(*it);

            // resolve symlinks, happens eg. with Maemo optify
            if (fileInfo.canonicalFilePath().isEmpty()) {
                continue;
            }

            const QFileInfo canonicalFileInfo(fileInfo.canonicalFilePath());

            if (canonicalFileInfo.exists() && canonicalFileInfo.isFile() && canonicalFileInfo.isExecutable()) {
                return *it;
            }
        }
    }

    return QString();
}

QStringList XdgBaseDirs::findPluginDirs()
{
    if (instance()->mPluginDirs.isEmpty()) {
#if QT_VERSION < 0x050000
        QStringList pluginDirs = instance()->systemPathList("QT_PLUGIN_PATH", AKONADILIB ":" AKONADILIB "/qt4/plugins/:" AKONADILIB "/kde4/:" AKONADILIB "/kde4/plugins/:/usr/lib/qt4/plugins/");
#else
        QStringList pluginDirs = instance()->systemPathList("QT_PLUGIN_PATH", AKONADILIB ":" AKONADILIB "/qt5/plugins/:" AKONADILIB "/kf5/:" AKONADILIB "/kf5/plugins/:/usr/lib/qt5/plugins/");
#endif
        if (QCoreApplication::instance() != 0) {
            Q_FOREACH (const QString &libraryPath, QCoreApplication::instance()->libraryPaths()) {
                if (!pluginDirs.contains(libraryPath)) {
                    pluginDirs << libraryPath;
                }
            }
        }
#if QT_VERSION < 0x050000
        // fallback for users with KDE in a different prefix and not correctly set up XDG_DATA_DIRS, hi David ;-)
        QProcess proc;
        // ### should probably rather be --path xdg-something
        const QStringList args = QStringList() << QLatin1String("--path") << QLatin1String("module");
        proc.start(QLatin1String("kde4-config"), args);
        if (proc.waitForStarted() && proc.waitForFinished() && proc.exitCode() == 0) {
            proc.setReadChannel(QProcess::StandardOutput);
            Q_FOREACH (const QString &path, splitPathList(QString::fromLocal8Bit(proc.readLine().trimmed()))) {
                if (!pluginDirs.contains(path)) {
                    pluginDirs.append(path);
                }
            }
        }
#endif
        qWarning() << "search paths: " << pluginDirs;
        instance()->mPluginDirs = pluginDirs;
    }

    return instance()->mPluginDirs;
}

QString XdgBaseDirs::findPluginFile(const QString &relPath, const QStringList &searchPath)
{
    const QStringList searchDirs = findPluginDirs() + searchPath;

#if defined(Q_OS_WIN) //krazy:exclude=cpp
    const QString pluginName = relPath + QLatin1String(".dll");
#else
    const QString pluginName = relPath + QLatin1String(".so");
#endif

    Q_FOREACH (const QString &path, searchDirs) {
        const QFileInfo fileInfo(path + QDir::separator() + pluginName);

        // resolve symlinks, happens eg. with Maemo optify
        if (fileInfo.canonicalFilePath().isEmpty()) {
            continue;
        }

        const QFileInfo canonicalFileInfo(fileInfo.canonicalFilePath());
        if (canonicalFileInfo.exists() && canonicalFileInfo.isFile()) {
            return canonicalFileInfo.absoluteFilePath();
        }
    }

    return QString();
}

QString XdgBaseDirs::findResourceDir(const char *resource, const QString &relPath)
{
    QString fullPath = homePath(resource) + QLatin1Char('/') + relPath;

    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable()) {
        return fullPath;
    }

    Q_FOREACH (const QString &path, systemPathList(resource)) {
        fileInfo = QFileInfo(path + QLatin1Char('/') + relPath);
        if (fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable()) {
            return fileInfo.absoluteFilePath();
        }
    }

    return QString();
}

QStringList XdgBaseDirs::findAllResourceDirs(const char *resource, const QString &relPath)
{
    QStringList resultList;

    const QString fullPath = homePath(resource) + QLatin1Char('/') + relPath;

    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable()) {
        resultList << fileInfo.absoluteFilePath();
    }

    Q_FOREACH (const QString &path, systemPathList(resource)) {
        fileInfo = QFileInfo(path + QLatin1Char('/') + relPath);
        if (fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable()) {
            const QString absPath = fileInfo.absoluteFilePath();
            if (!resultList.contains(absPath)) {
                resultList << absPath;
            }
        }
    }

    return resultList;
}

QString XdgBaseDirs::saveDir(const char *resource, const QString &relPath)
{
    const QString fullPath = homePath(resource) + QLatin1Char('/') + relPath;

    QFileInfo fileInfo(fullPath);
    if (fileInfo.exists()) {
        if (fileInfo.isDir()) {
            return fullPath;
        } else {
            qWarning() << "XdgBaseDirs::saveDir: '" << fileInfo.absoluteFilePath()
                       << "' exists but is not a directory";
        }
    } else {
        if (!QDir::home().mkpath(fileInfo.absoluteFilePath())) {
            qWarning() << "XdgBaseDirs::saveDir: failed to create directory '"
                       << fileInfo.absoluteFilePath() << "'";
        } else {
            return fullPath;
        }
    }

    return QString();
}

QString XdgBaseDirs::akonadiServerConfigFile(FileAccessMode openMode)
{
    return akonadiConfigFile(QLatin1String("akonadiserverrc"), openMode);
}

QString XdgBaseDirs::akonadiConnectionConfigFile(FileAccessMode openMode)
{
    return akonadiConfigFile(QLatin1String("akonadiconnectionrc"), openMode);
}

QString XdgBaseDirs::akonadiConfigFile(const QString &file, FileAccessMode openMode)
{
    const QString akonadiDir = QLatin1String("akonadi");

    const QString savePath = saveDir("config", akonadiDir) + QLatin1Char('/') + file;

    if (openMode == WriteOnly) {
        return savePath;
    }

    const QString path = findResourceFile("config", akonadiDir + QLatin1Char('/') + file);

    if (path.isEmpty()) {
        return savePath;
    } else if (openMode == ReadOnly || path == savePath) {
        return path;
    }

    // file found in system paths and mode is ReadWrite, thus
    // we copy to the home path location and return this path
    QFile systemFile(path);

    systemFile.copy(savePath);

    return savePath;
}

QString XdgBaseDirsSingleton::homePath(const char *variable, const char *defaultSubDir)
{
    const QByteArray env = qgetenv(variable);

    QString xdgPath;
    if (env.isEmpty()) {
        xdgPath = QDir::homePath() + QLatin1Char('/') + QLatin1String(defaultSubDir);
#if defined(Q_OS_WIN) //krazy:exclude=cpp
    } else if (QDir::isAbsolutePath(QString::fromLocal8Bit(env))) {
#else
    } else if (env.startsWith('/')) {
#endif
        xdgPath = QString::fromLocal8Bit(env);
    } else {
        xdgPath = QDir::homePath() + QLatin1Char('/') + QString::fromLocal8Bit(env);
    }

    return xdgPath;
}

QStringList XdgBaseDirsSingleton::systemPathList(const char *variable, const char *defaultDirList)
{
    const QByteArray env = qgetenv(variable);

    QString xdgDirList;
    if (env.isEmpty()) {
        xdgDirList = QLatin1String(defaultDirList);
    } else {
        xdgDirList = QString::fromLocal8Bit(env);
    }

    return splitPathList(xdgDirList);
}
