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

#ifndef AKSTANDARDDIRS_H
#define AKSTANDARDDIRS_H

#include "xdgbasedirs_p.h"
#include "akonadiprivate_export.h"

namespace Akonadi
{

/**
 * Convenience wrappers on top of XdgBaseDirs that are instance namespace aware.
 * @since 1.7
 */
namespace StandardDirs
{
/**
 * Returns path to the config file @p configFile.
 */
AKONADIPRIVATE_EXPORT QString configFile(const QString &configFile, Akonadi::XdgBaseDirs::FileAccessMode openMode = Akonadi::XdgBaseDirs::ReadOnly);

/**
 * Returns the full path to the server config file (akonadiserverrc).
 */
AKONADIPRIVATE_EXPORT QString serverConfigFile(Akonadi::XdgBaseDirs::FileAccessMode openMode = Akonadi::XdgBaseDirs::ReadOnly);

/**
 * Returns the full path to the connection config file (akonadiconnectionrc).
 */
AKONADIPRIVATE_EXPORT QString connectionConfigFile(Akonadi::XdgBaseDirs::FileAccessMode openMode = Akonadi::XdgBaseDirs::ReadOnly);

/**
 * Returns the full path to the agent config file (agentsrc).
 */
AKONADIPRIVATE_EXPORT QString agentConfigFile(Akonadi::XdgBaseDirs::FileAccessMode openMode = Akonadi::XdgBaseDirs::ReadOnly);

/**
 * Instance-aware wrapper for XdgBaseDirs::saveDir().
 * @note @p relPath does not need to include the "akonadi/" folder.
 * @see XdgBaseDirs::saveDir()
 */
AKONADIPRIVATE_EXPORT QString saveDir(const char *resource, const QString &relPath = QString());

}
}

#endif
