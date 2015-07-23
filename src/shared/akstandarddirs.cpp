/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include "akstandarddirs.h"
#include "akapplication.h"

#include <private/xdgbasedirs_p.h>

#include <QFile>

using namespace Akonadi;

QString AkStandardDirs::configFile(const QString &configFile, Akonadi::XdgBaseDirs::FileAccessMode openMode)
{
    const QString savePath = AkStandardDirs::saveDir("config") + QLatin1Char('/') + configFile;

    if (openMode == XdgBaseDirs::WriteOnly) {
        return savePath;
    }

    QString path = XdgBaseDirs::findResourceFile("config", QLatin1String("akonadi/") + configFile);
    // HACK: when using instance namespaces, ignore the non-namespaced file
    if (AkApplication::hasInstanceIdentifier() && path.startsWith(XdgBaseDirs::homePath("config"))) {
        path.clear();
    }

    if (path.isEmpty()) {
        return savePath;
    } else if (openMode == XdgBaseDirs::ReadOnly || path == savePath) {
        return path;
    }

    // file found in system paths and mode is ReadWrite, thus
    // we copy to the home path location and return this path
    QFile systemFile(path);

    systemFile.copy(savePath);

    return savePath;
}

QString AkStandardDirs::serverConfigFile(XdgBaseDirs::FileAccessMode openMode)
{
    return configFile(QStringLiteral("akonadiserverrc"), openMode);
}

QString AkStandardDirs::connectionConfigFile(XdgBaseDirs::FileAccessMode openMode)
{
    return configFile(QStringLiteral("akonadiconnectionrc"), openMode);
}

QString AkStandardDirs::agentConfigFile(XdgBaseDirs::FileAccessMode openMode)
{
    return configFile(QStringLiteral("agentsrc"), openMode);
}

QString AkStandardDirs::saveDir(const char *resource, const QString &relPath)
{
    QString fullRelPath = QStringLiteral("akonadi");
    if (AkApplication::hasInstanceIdentifier()) {
        fullRelPath += QLatin1String("/instance/") + AkApplication::instanceIdentifier();
    }
    if (!relPath.isEmpty()) {
        fullRelPath += QLatin1Char('/') + relPath;
    }
    return XdgBaseDirs::saveDir(resource, fullRelPath);
}
