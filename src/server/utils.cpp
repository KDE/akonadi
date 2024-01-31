/*
 * SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 */

#include "utils.h"
#include "akonadiserver_debug.h"
#include "instance_p.h"

#include "private/standarddirs_p.h"

#include <QDir>
#include <QFileInfo>
#include <QHostInfo>
#include <QSettings>

#if !defined(Q_OS_WIN)
#include <cerrno>
#include <cstdlib>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

static QString akonadiSocketDirectory();
static bool checkSocketDirectory(const QString &path);
static bool createSocketDirectory(const QString &link);
#endif

#ifdef Q_OS_LINUX
#include <fcntl.h>
#include <mntent.h>
#include <stdio.h>
#include <sys/ioctl.h>
#endif

using namespace Akonadi;
using namespace Akonadi::Server;

QString Utils::preferredSocketDirectory(const QString &defaultDirectory, int fnLengthHint)
{
    const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadWrite);
    const QSettings serverSettings(serverConfigFile, QSettings::IniFormat);

#if defined(Q_OS_WIN)
    const QString socketDir = serverSettings.value(QLatin1StringView("Connection/SocketDirectory"), defaultDirectory).toString();
#else
    QString socketDir = defaultDirectory;
    if (!serverSettings.contains(QStringLiteral("Connection/SocketDirectory"))) {
        // if no socket directory is defined, use the symlinked from /tmp
        socketDir = akonadiSocketDirectory();

        if (socketDir.isEmpty()) { // if that does not work, fall back on default
            socketDir = defaultDirectory;
        }
    } else {
        socketDir = serverSettings.value(QStringLiteral("Connection/SocketDirectory"), defaultDirectory).toString();
    }

    if (socketDir.contains(QLatin1StringView("$USER"))) {
        const QString userName = QString::fromLocal8Bit(qgetenv("USER"));
        if (!userName.isEmpty()) {
            socketDir.replace(QLatin1StringView("$USER"), userName);
        }
    }

    if (socketDir[0] != QLatin1Char('/')) {
        QDir::home().mkdir(socketDir);
        socketDir = QDir::homePath() + QLatin1Char('/') + socketDir;
    }

    QFileInfo dirInfo(socketDir);
    if (!dirInfo.exists()) {
        QDir::home().mkpath(dirInfo.absoluteFilePath());
    }

    const std::size_t totalLength = socketDir.length() + 1 + fnLengthHint;
    const std::size_t maxLen = sizeof(sockaddr_un::sun_path);
    if (totalLength >= maxLen) {
        qCCritical(AKONADISERVER_LOG) << "akonadiSocketDirectory() length of" << totalLength << "is longer than the system limit" << maxLen;
    }
#endif
    return socketDir;
}

#if !defined(Q_OS_WIN)
QString akonadiSocketDirectory()
{
    const QString hostname = QHostInfo::localHostName();

    if (hostname.isEmpty()) {
        qCCritical(AKONADISERVER_LOG) << "QHostInfo::localHostName() failed";
        return QString();
    }

    const QString identifier = Instance::hasIdentifier() ? Instance::identifier() : QStringLiteral("default");
    const QString link = StandardDirs::saveDir("data") + QStringLiteral("/socket-%1-%2").arg(hostname, identifier);

    if (checkSocketDirectory(link)) {
        return QFileInfo(link).symLinkTarget();
    }

    if (createSocketDirectory(link)) {
        return QFileInfo(link).symLinkTarget();
    }

    qCCritical(AKONADISERVER_LOG) << "Could not create socket directory for Akonadi.";
    return QString();
}

static bool checkSocketDirectory(const QString &path)
{
    QFileInfo info(path);

    if (!info.exists()) {
        return false;
    }

    if (info.isSymLink()) {
        info = QFileInfo(info.symLinkTarget());
    }

    if (!info.isDir()) {
        return false;
    }

    if (info.ownerId() != getuid()) {
        return false;
    }

    return true;
}

static bool createSocketDirectory(const QString &link)
{
    const QString directory = StandardDirs::saveDir("runtime");

    if (!QDir().mkpath(directory)) {
        qCCritical(AKONADISERVER_LOG) << "Creating socket directory with name" << directory << "failed:" << strerror(errno);
        return false;
    }

    QFile::remove(link);

    if (!QFile::link(directory, link)) {
        qCCritical(AKONADISERVER_LOG) << "Creating symlink from" << directory << "to" << link << "failed";
        return false;
    }

    return true;
}
#endif

QString Utils::getDirectoryFileSystem(const QString &directory)
{
#ifndef Q_OS_LINUX
    Q_UNUSED(directory)
    return QString();
#else
    QString bestMatchPath;
    QString bestMatchFS;

    FILE *mtab = setmntent("/etc/mtab", "r");
    if (!mtab) {
        return QString();
    }
    while (mntent *mnt = getmntent(mtab)) {
        if (qstrcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
            continue;
        }

        const QString dir = QString::fromLocal8Bit(mnt->mnt_dir);
        if (!directory.startsWith(dir) || dir.length() < bestMatchPath.length()) {
            continue;
        }

        bestMatchPath = dir;
        bestMatchFS = QString::fromLocal8Bit(mnt->mnt_type);
    }

    endmntent(mtab);

    return bestMatchFS;
#endif
}

void Utils::disableCoW(const QString &path)
{
#ifndef Q_OS_LINUX
    Q_UNUSED(path)
#else
    qCDebug(AKONADISERVER_LOG) << "Detected Btrfs, disabling copy-on-write on database files";

    // from linux/fs.h, so that Akonadi does not depend on Linux header files
#ifndef FS_IOC_GETFLAGS
#define FS_IOC_GETFLAGS _IOR('f', 1, long)
#endif
#ifndef FS_IOC_SETFLAGS
#define FS_IOC_SETFLAGS _IOW('f', 2, long)
#endif

    // Disable COW on file
#ifndef FS_NOCOW_FL
#define FS_NOCOW_FL 0x00800000
#endif

    ulong flags = 0;
    const int fd = open(qPrintable(path), O_RDONLY);
    if (fd == -1) {
        qCWarning(AKONADISERVER_LOG) << "Failed to open" << path << "to modify flags (" << errno << ")";
        return;
    }

    if (ioctl(fd, FS_IOC_GETFLAGS, &flags) == -1) {
        qCWarning(AKONADISERVER_LOG) << "ioctl error: failed to get file flags (" << errno << ")";
        close(fd);
        return;
    }
    if (!(flags & FS_NOCOW_FL)) {
        flags |= FS_NOCOW_FL;
        if (ioctl(fd, FS_IOC_SETFLAGS, &flags) == -1) {
            qCWarning(AKONADISERVER_LOG) << "ioctl error: failed to set file flags (" << errno << ")";
            close(fd);
            return;
        }
    }
    close(fd);
#endif
}
