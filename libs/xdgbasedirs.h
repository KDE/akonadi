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

#ifndef XDGBASEDIRS_H
#define XDGBASEDIRS_H

#include "libakonadi_export.h"

// Qt includes
#include <QtCore/QFlags>

// forward declarations
class QString;
class QStringList;

namespace Akonadi {

class XdgBaseDirsPrivate;

/**
 @brief Resource type based handling of standard directories

 Developers of several Free Software desktop projects have created
 a specification for handling so-called "base directories", i.e.
 lists of system wide directories and directories within each user's
 home directory for installing and later finding certain files.

 This class handles the respective behaviour, i.e. environment variables
 and their defaults, for the following type of resources:
 - "config"
 - "data"

 Example: getting the Akonadi server config file "akonadiserverrc", assuming
 that Akonadi stores its config in an additional subdirectoy called "akonadi"
 @code
 QString relativeFileName = QLatin1String( "akonadi/akonadiserverrc" );

 // look for the file "akonadiserverrc" with additional subdirectory "akonadi"
 // in any directory associated with resource type "config"
 QString configFile = XdgBaseDirs::findResourceFile( "config", relativeFileName );

 if ( configFile.isEmpty() ) {
  // No config file yet, get the suitable user specific directory for storing
  // a new one
  configFile  = XdgBaseDirs::saveDir( "config", QLatin1String( "akonadi" ) );
  configFile += QLatin1String( "akonadiserverrc" );
 }

 QSettings serverConfig( configFile );
 @endcode

 @author Kevin Krammer, <kevin.krammer@gmx.at>

 @see http://www.freedesktop.org/wiki/Specifications/basedir-spec
 */
class AKONADI_EXPORT XdgBaseDirs
{
  public:
    /**
     @brief Creates the instance
     */
    XdgBaseDirs();

    /**
     @brief Destroys the instance
     */
    ~XdgBaseDirs();

    /**
     @brief Returns the user specific directory for the given resource type

     Unless the user's environment has a specific path set as an override
     this will be the default as defined in the freedesktop.org base-dir-spec

     @note Caches the value of the first call

     @param resource a named resource type, e.g. "config"

     @return a directory path

     @see systemPathList()
     @see saveDir()
     */
    static QString homePath( const char *resource );

    /**
     @brief Returns the list of system wide directories for a given resource type

     The returned list can contain one or more directory paths. If there are more
     than one, the list is sorted by falling priority, i.e. if an entry is valid
     for the respective use case (e.g. contains a file the application looks for)
     the list should not be processed further.

     @note The user's resource path should, to be compliant with the spec,
           always be treated as having higher priority than any path in the
           list of system wide paths

     @note Caches the value of the first call

     @param resource a named resource type, e.g. "config"

     @return a priority sorted list of directory paths

     @see homePath()
     */
    static QStringList systemPathList( const char *resource );

    /**
     @brief Searches the resource specific directories for a given file

     Convenience method for finding a given file (with optional relative path)
     in any of the configured base directories for a given resource type.

     Will check the user local directory first and then process the system
     wide path list according to the inherent priority.

     @param resource a named resource type, e.g. "config"
     @param relPath relative path of a file to look for,
            e.g."akonadi/akonadiserverrc"

     @returns the file path of the first match, or @c QString() if no such
              relative path exists in any of the base directories or if
              a match is not a file

     @see findResourceDir()
     @see saveDir
     */
    static QString findResourceFile( const char *resource, const QString &relPath );

    /**
     @brief Searches the executable specific directories for a given file

     Convenience method for finding a given executable (with optional relative path)
     in any of the configured directories for a this special type.

     @note This is not based on the XDG base dir spec, since it does not cover
           executable

     @param relPath relative path of a file to look for,
            e.g."akonadiserver"

     @returns the file path of the first match, or @c QString() if no such
              relative path exists in any of the base directories

     @see findResourceFile()
     */
    static QString findExecutableFile( const QString &relPath );

    /**
     @brief Searches the resource specific directories for a given subdirectory

     Convenience method for finding a given relative subdirectory in any of
     the configured base directories for a given resource type.

     Will check the user local directory first and then process the system
     wide path list according to the inherent priority.

     Use findAllResourceDirs() if looking for all directories with the given
     subdirectory.

     @param resource a named resource type, e.g. "config"
     @param relPath relative path of a subdirectory to look for,
            e.g."akonadi/agents"

     @returns the directory path of the first match, or @c QString() if no such
              relative path exists in any of the base directories or if
              a match is not a directory

     @see findResourceFile()
     @see saveDir()
     */
    static QString findResourceDir( const char *resource, const QString &relPath );

    /**
     @brief Searches the resource specific directories for a given subdirectory

     Convenience method for getting a list of directoreis with a given relative
     subdirectory in any of the configured base directories for a given
     resource type.

     Will check the user local directory first and then process the system
     wide path list according to the inherent priority.

     Similar to findResourceDir() but does not just find the first best match
     but all matching resource directories. The resuling list will be sorted
     according to the same proprity criteria.

     @param resource a named resource type, e.g. "config"
     @param relPath relative path of a subdirectory to look for,
            e.g."akonadi/agents"

     @returns a list of directory paths, or @c QString() if no such
              relative path exists in any of the base directories or if
              non of the matches is a directory

     @see findResourceDir()
     */
    static QStringList findAllResourceDirs( const char *resource, const QString &relPath );

    /**
     @brief Finds or creates the "save to" directory for a given resource

     Convenience method for creating subdirectores relative to a given
     resource type's user directory, i.e. homePath() + relPath

     If the target directory does not exists, it an all necessary parent
     directories will be created, unless denied by the filesystem.

     @param resource a named resource type, e.g. "config"
     @param relPath relative path of a directory to be used for file writing

     @return the directory path of the "save to" directory or @c QString()
             if the directory or one of its parents could not be created

     @see findResourceDir()
     */
    static QString saveDir( const char *resource, const QString &relPath );

    /**
    * @brief Open mode flags for resource files
    *
    * FileAccessMode is a typedef for QFlags<FileAccessFlag>. It stores
    * a OR combination of FileAccessFlag values
    */
    enum FileAccessFlag
    {
        ReadOnly  = 0x1,
        WriteOnly = 0x2,
        ReadWrite = ReadOnly | WriteOnly
    };

    typedef QFlags<FileAccessFlag> FileAccessMode;

    /**
    * @brief Returns the path of the Akonadi server config file
    *
    * Convenience method for getting the server config file "akonadiserverrc"
    * since this is an often needed procedure in several parts of the code.
    *
    * @param openMode how the application wants to use the config file
    *
    * @return the path of the server config file, suitable for \p openMode
    */
    static QString akonadiServerConfigFile( FileAccessMode openMode = ReadOnly );

    /**
    * @brief Returns the path of the Akonadi data connection config file
    *
    * Convenience method for getting the server config file "akonadiconnectionrc"
    * since this is an often needed procedure in several parts of the code.
    *
    * @param openMode how the application wants to use the config file
    *
    * @return the path of the data connection config file, suitable for \p openMode
    */
    static QString akonadiConnectionConfigFile( FileAccessMode openMode = ReadOnly );

  private:
    XdgBaseDirsPrivate* const d;

  private:
    static QString akonadiConfigFile( const QString &file, FileAccessMode openMode );

  private:
    XdgBaseDirs( const XdgBaseDirs &);
    XdgBaseDirs &operator=( const XdgBaseDirs &);
};

}

#endif
