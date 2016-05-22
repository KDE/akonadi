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

#include "akonadiprivate_debug.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>
#include <QtCore/QProcessEnvironment>
#include <QtCore/QRegExp>

#include <cstdlib>

#ifdef Q_OS_WIN
# include <windows.h>
#endif

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

#ifdef Q_OS_WIN
static QMap<QString, QString> getEnvironment()
{
    QMap<QString, QString> ret;
    Q_FOREACH(const QString& str, QProcessEnvironment::systemEnvironment().toStringList())
    {
        const int p = str.indexOf(QLatin1Char('='));
        ret[str.left(p)] = str.mid(p + 1);
    }
    return ret;
}

QString expandEnvironmentVariables(const QString& str)
{
    static QMap<QString, QString> envVars = getEnvironment();
    static QRegExp possibleVars(QLatin1String("((\\{|%)(\\w+)(\\}|%))"));
    QString ret = str;
    while(possibleVars.indexIn(ret) != -1)
    {
        QStringList caps = possibleVars.capturedTexts();
        if(caps[2] == QLatin1String("{"))
        {
            ret.replace(QLatin1String("$") + caps[1], envVars[caps[3]]);
        }
        else
        {
            ret.replace(caps[1], envVars[caps[3]]);
        }
        QString key = possibleVars.cap();
        ret.replace(key, envVars[key]);
    }
    return ret;
}

static QSettings* getKdeConf()
{
    WCHAR wPath[MAX_PATH+1];
    GetModuleFileNameW(NULL, wPath, MAX_PATH);
    QString kdeconfPath = QString::fromUtf16((const ushort *) wPath);
    kdeconfPath = kdeconfPath.left(kdeconfPath.lastIndexOf(QLatin1Char('\\'))).replace(QLatin1Char('\\'), QLatin1Char('/'));
    if(QFile::exists(kdeconfPath + QString::fromLatin1("/kde.conf")))
    {
        return new QSettings(kdeconfPath + QString::fromLatin1("/kde.conf"), QSettings::IniFormat);
    }
    else
    {
        return 0;
    }
}
#endif

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
#ifdef Q_OS_WIN
    static QSettings* kdeconf = getKdeConf();
#endif
    if (qstrncmp("data", resource, 4) == 0) {
        if (instance()->mDataHome.isEmpty()) {
#ifdef Q_OS_WIN
            if(kdeconf) {
                kdeconf->beginGroup(QLatin1String("XDG"));
                if(kdeconf->childKeys().contains(QLatin1String("XDG_DATA_HOME")))
                    instance()->mDataHome = expandEnvironmentVariables(kdeconf->value(QLatin1String("XDG_DATA_HOME")).toString());
                else
                    instance()->mDataHome = instance()->homePath("XDG_DATA_HOME", ".local/share");
                kdeconf->endGroup();
            } else {
#else
            {
#endif
                instance()->mDataHome = instance()->homePath( "XDG_DATA_HOME", ".local/share" );
            }
        }
        return instance()->mDataHome;
    } else if (qstrncmp("config", resource, 6) == 0) {
        if (instance()->mConfigHome.isEmpty()) {
#ifdef Q_OS_WIN
            if(kdeconf) {
                kdeconf->beginGroup(QLatin1String("XDG"));
                if(kdeconf->childKeys().contains(QLatin1String("XDG_CONFIG_HOME")))
                    instance()->mConfigHome = expandEnvironmentVariables(kdeconf->value(QLatin1String("XDG_CONFIG_HOME")).toString());
                else
                    instance()->mConfigHome = instance()->homePath( "XDG_CONFIG_HOME", ".config" );
                kdeconf->endGroup();
            } else {
#else
            {
#endif
                instance()->mConfigHome = instance()->homePath( "XDG_CONFIG_HOME", ".config" );
            }
        }
        return instance()->mConfigHome;
    }

    return QString();
}

QStringList XdgBaseDirs::systemPathList(const char *resource)
{
#ifdef Q_OS_WIN
    static QSettings* kdeconf = getKdeConf();
#endif
    if (qstrncmp("data", resource, 4) == 0) {
        if (instance()->mDataDirs.isEmpty()) {
#ifdef Q_OS_WIN
            QStringList dataDirs;
            if(kdeconf) {
                kdeconf->beginGroup(QLatin1String("XDG"));
                if(kdeconf->childKeys().contains(QLatin1String("XDG_DATA_DIRS"))) {
                dataDirs = instance()->systemPathList( "XDG_DATA_DIRS", expandEnvironmentVariables(kdeconf->value(QLatin1String("XDG_DATA_DIRS")).toString()).toLocal8Bit().constData() );
                } else {
                    QDir dir(QCoreApplication::applicationDirPath());
                    dir.cdUp();
                    const QString defaultPathList = dir.absoluteFilePath(QLatin1String("share"));
                    dataDirs = instance()->systemPathList( "XDG_DATA_DIRS", defaultPathList.toLocal8Bit().constData() );
                }
                kdeconf->endGroup();
            } else {
                QDir dir( QCoreApplication::applicationDirPath() );
                dir.cdUp();
                const QString defaultPathList = dir.absoluteFilePath( QLatin1String( "share" ) );
                dataDirs = instance()->systemPathList( "XDG_DATA_DIRS", defaultPathList.toLocal8Bit().constData() );
            }
#else
            QStringList dataDirs = instance()->systemPathList("XDG_DATA_DIRS", "/usr/local/share:/usr/share");
#endif

#ifdef Q_OS_WIN
            const QString prefixDataDir = QLatin1String(AKONADIPREFIX "/" AKONADIDATA);
#else
            const QString prefixDataDir = QStringLiteral(AKONADIDATA);
#endif
            if (!dataDirs.contains(prefixDataDir)) {
                dataDirs << prefixDataDir;
            }

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
            QStringList configDirs;
            if(kdeconf) {
                kdeconf->beginGroup(QLatin1String("XDG"));
                if(kdeconf->childKeys().contains(QLatin1String("XDG_CONFIG_DIRS"))) {
                    configDirs = instance()->systemPathList( "XDG_CONFIG_DIRS", expandEnvironmentVariables(kdeconf->value(QLatin1String("XDG_CONFIG_DIRS")).toString()).toLocal8Bit().constData() );
                } else {
                    QDir dir(QCoreApplication::applicationDirPath());
                    dir.cdUp();
                    const QString defaultPathList = dir.absoluteFilePath(QLatin1String("etc")) + QLatin1Char(';') + dir.absoluteFilePath(QLatin1String("share/config"));
                    configDirs = instance()->systemPathList( "XDG_CONFIG_DIRS", defaultPathList.toLocal8Bit().constData() );
                }
                kdeconf->endGroup();
            } else {
                QDir dir( QCoreApplication::applicationDirPath() );
                dir.cdUp();
                const QString defaultPathList = dir.absoluteFilePath( QLatin1String( "etc" ) ) + QLatin1Char( ';' ) + dir.absoluteFilePath( QLatin1String( "share/config" ) );
                configDirs = instance()->systemPathList( "XDG_CONFIG_DIRS", defaultPathList.toLocal8Bit().constData() );
            }
#else
            QStringList configDirs = instance()->systemPathList("XDG_CONFIG_DIRS", "/etc/xdg");
#endif

#ifdef Q_OS_WIN
            const QString prefixConfigDir = QLatin1String(AKONADIPREFIX "/" AKONADICONFIG);
#else
            const QString prefixConfigDir = QStringLiteral(AKONADICONFIG);
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

        const QString prefixExecutableDir = QStringLiteral(AKONADIPREFIX "/bin");
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
        qCDebug(AKONADIPRIVATE_LOG) << "search paths: " << executableDirs;

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
        QStringList pluginDirs = instance()->systemPathList("QT_PLUGIN_PATH", AKONADILIB ":" AKONADILIB "/qt5/plugins/:" AKONADILIB "/kf5/:" AKONADILIB "/kf5/plugins/:/usr/lib/qt5/plugins/");
        if (QCoreApplication::instance() != 0) {
            Q_FOREACH (const QString &libraryPath, QCoreApplication::instance()->libraryPaths()) {
                if (!pluginDirs.contains(libraryPath)) {
                    pluginDirs << libraryPath;
                }
            }
        }
        qCDebug(AKONADIPRIVATE_LOG) << "search paths: " << pluginDirs;
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
            qCWarning(AKONADIPRIVATE_LOG) << "XdgBaseDirs::saveDir: '" << fileInfo.absoluteFilePath()
                                          << "' exists but is not a directory";
        }
    } else {
        if (!QDir::home().mkpath(fileInfo.absoluteFilePath())) {
            qCWarning(AKONADIPRIVATE_LOG) << "XdgBaseDirs::saveDir: failed to create directory '"
                                          << fileInfo.absoluteFilePath() << "'";
        } else {
            return fullPath;
        }
    }

    return QString();
}

QString XdgBaseDirs::akonadiServerConfigFile(FileAccessMode openMode)
{
    return akonadiConfigFile(QStringLiteral("akonadiserverrc"), openMode);
}

QString XdgBaseDirs::akonadiConnectionConfigFile(FileAccessMode openMode)
{
    return akonadiConfigFile(QStringLiteral("akonadiconnectionrc"), openMode);
}

QString XdgBaseDirs::akonadiConfigFile(const QString &file, FileAccessMode openMode)
{
    const QString akonadiDir = QStringLiteral("akonadi");

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
