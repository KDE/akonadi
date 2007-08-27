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

#include "xdgbasedirs.h"

#include "akonadi-prefix.h" // for prefix defines

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>

#include <cstdlib>

static QStringList alternateExecPaths( const QString& path )
{
  QStringList pathList;

  pathList << path;

#if defined(Q_OS_WIN)
  pathList << path + QLatin1String( ".exe" );
#elif defined(Q_OS_MAC)
  pathList << path + QLatin1String( ".app/Contents/MacOS/" ) + path.section( QLatin1Char( '/' ), -1 );
#endif

  return pathList;
}

namespace Akonadi {

class XdgBaseDirsPrivate
{
  public:
    XdgBaseDirsPrivate() {}

    ~XdgBaseDirsPrivate() {}
};

class XdgBaseDirsSingleton
{
public:
    QString homePath( const char *variable, const char *defaultSubDir );

    QStringList systemPathList( const char *variable, const char *defaultDirList );

  public:
    QString mConfigHome;
    QString mDataHome;

    QStringList mConfigDirs;
    QStringList mDataDirs;
    QStringList mExecutableDirs;
};

Q_GLOBAL_STATIC(XdgBaseDirsSingleton, instance);

}

using namespace Akonadi;

XdgBaseDirs::XdgBaseDirs() : d( new XdgBaseDirsPrivate() )
{
}

XdgBaseDirs::~XdgBaseDirs()
{
  delete d;
}

QString XdgBaseDirs::homePath( const char *resource )
{
  if ( qstrncmp( "data", resource, 4 ) == 0 ) {
    if ( instance()->mDataHome.isEmpty() ) {
      instance()->mDataHome = instance()->homePath( "XDG_DATA_HOME", ".local/share" );
    }
    return instance()->mDataHome;
  } else if ( qstrncmp( "config", resource, 6 ) == 0 ) {
    if ( instance()->mConfigHome.isEmpty() ) {
      instance()->mConfigHome = instance()->homePath( "XDG_CONFIG_HOME", ".config" );
    }
    return instance()->mConfigHome;
  }

  return QString();
}

QStringList XdgBaseDirs::systemPathList( const char *resource )
{
  if ( qstrncmp( "data", resource, 4 ) == 0 ) {
    if ( instance()->mDataDirs.isEmpty() ) {
      QStringList dataDirs = instance()->systemPathList( "XDG_DATA_DIRS", "/usr/local/share:/usr/share" );

      const QString prefixDataDir = QLatin1String( AKONADIDATA );
      if ( !dataDirs.contains( prefixDataDir ) ) {
        dataDirs << prefixDataDir;
      }

      instance()->mDataDirs = dataDirs;
    }
    return instance()->mDataDirs;
  } else if ( qstrncmp( "config", resource, 6 ) == 0 ) {
    if ( instance()->mConfigDirs.isEmpty() ) {
      QStringList configDirs = instance()->systemPathList( "XDG_CONFIG_DIRS", "/etc/xdg" );

      const QString prefixConfigDir = QLatin1String( AKONADICONFIG );
      if ( !configDirs.contains( prefixConfigDir ) ) {
        configDirs << prefixConfigDir;
      }

      instance()->mConfigDirs = configDirs;
    }
    return instance()->mConfigDirs;
  }

  return QStringList();
}

QString XdgBaseDirs::findResourceFile( const char *resource, const QString &relPath )
{
  QString fullPath = homePath( resource ) + QLatin1Char('/' ) + relPath;

  QFileInfo fileInfo( fullPath );
  if ( fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable() ) {
    return fullPath;
  }

  QStringList pathList = systemPathList( resource );

  QStringList::const_iterator it    = pathList.begin();
  QStringList::const_iterator endIt = pathList.end();
  for ( ; it != endIt; ++it ) {
    fileInfo = QFileInfo( *it + QLatin1Char('/' ) + relPath );
    if ( fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable() ) {
      return fileInfo.absoluteFilePath();
    }
  }

  return QString();
}

QString XdgBaseDirs::findExecutableFile( const QString &relPath )
{
  if ( instance()->mExecutableDirs.isEmpty() ) {
    QStringList executableDirs = instance()->systemPathList( "PATH", "/usr/local/bin:/usr/bin" );

    const QString prefixExecutableDir = QLatin1String( AKONADIPREFIX "/bin" );
    if ( !executableDirs.contains( prefixExecutableDir ) ) {
      executableDirs << prefixExecutableDir;
    }

    instance()->mExecutableDirs = executableDirs;
  }

  QStringList::const_iterator pathIt    = instance()->mExecutableDirs.begin();
  QStringList::const_iterator pathEndIt = instance()->mExecutableDirs.end();
  for ( ; pathIt != pathEndIt; ++pathIt ) {
    QStringList fullPathList = alternateExecPaths(*pathIt + QLatin1Char( '/' ) + relPath );

    QStringList::const_iterator it    = fullPathList.begin();
    QStringList::const_iterator endIt = fullPathList.end();
    for ( ; it != endIt; ++it ) {
      QFileInfo fileInfo(*it);

      if ( fileInfo.exists() && fileInfo.isFile() && fileInfo.isExecutable() ) {
        return *it;
      }
    }
  }

  return QString();
}

QString XdgBaseDirs::findResourceDir( const char *resource, const QString &relPath )
{
  QString fullPath = homePath( resource ) + QLatin1Char( '/' ) + relPath;

  QFileInfo fileInfo( fullPath );
  if ( fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable() ) {
    return fullPath;
  }

  QStringList pathList = systemPathList( resource );

  QStringList::const_iterator it    = pathList.begin();
  QStringList::const_iterator endIt = pathList.end();
  for ( ; it != endIt; ++it ) {
    fileInfo = QFileInfo( *it + QLatin1Char('/' ) + relPath );
    if ( fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable() ) {
      return fileInfo.absoluteFilePath();
    }
  }

  return QString();
}

QStringList XdgBaseDirs::findAllResourceDirs( const char *resource, const QString &relPath )
{
  QStringList resultList;

  QString fullPath = homePath( resource ) + QLatin1Char('/' ) + relPath;

  QFileInfo fileInfo( fullPath );
  if ( fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable() ) {
    resultList << fileInfo.absoluteFilePath();
  }

  QStringList pathList = systemPathList( resource );

  QStringList::const_iterator it    = pathList.begin();
  QStringList::const_iterator endIt = pathList.end();
  for ( ; it != endIt; ++it ) {
    fileInfo = QFileInfo( *it + QLatin1Char('/' ) + relPath );
    if ( fileInfo.exists() && fileInfo.isDir() && fileInfo.isReadable() ) {
      resultList << fileInfo.absoluteFilePath();
    }
  }

  return resultList;
}

QString XdgBaseDirs::saveDir( const char *resource, const QString &relPath )
{
  QString fullPath = homePath( resource ) + QLatin1Char('/' ) + relPath;

  QFileInfo fileInfo( fullPath );
  if ( fileInfo.exists() ) {
    if ( fileInfo.isDir() ) {
      return fullPath;
    } else {
      qWarning() << "XdgBaseDirs::saveDir: '" << fileInfo.absoluteFilePath()
                 << "' exists but is not a directory";
    }
  } else {
    if ( !QDir::home().mkpath( fileInfo.absoluteFilePath() ) ) {
      qWarning() << "XdgBaseDirs::saveDir: failed to create directory '"
                 << fileInfo.absoluteFilePath() << "'";
    } else {
      return fullPath;
    }
  }

  return QString();
}

QString XdgBaseDirs::akonadiServerConfigFile( FileAccessMode openMode )
{
  return akonadiConfigFile( QLatin1String( "akonadiserverrc" ), openMode );
}

QString XdgBaseDirs::akonadiConnectionConfigFile( FileAccessMode openMode )
{
  return akonadiConfigFile( QLatin1String( "akonadiconnectionrc" ), openMode );
}

QString XdgBaseDirs::akonadiConfigFile( const QString &file, FileAccessMode openMode )
{
  const QString akonadiDir = QLatin1String( "akonadi" );

  QString savePath = saveDir( "config", akonadiDir ) + QLatin1Char( '/' ) + file;

  if ( openMode == WriteOnly ) return savePath;

  QString path = findResourceFile( "config", akonadiDir + QLatin1Char( '/' ) + file );

  if ( path.isEmpty() ) {
    return savePath;
  } else if ( openMode == ReadOnly || path == savePath ) {
    return path;
  }

  // file found in system paths and mode is ReadWrite, thus
  // we copy to the home path location and return this path
  QFile systemFile( path );

  systemFile.copy( savePath );

  return savePath;
}

QString XdgBaseDirsSingleton::homePath( const char *variable, const char *defaultSubDir )
{
  char *env = std::getenv( variable );

  QString xdgPath;
  if ( env == 0 || *env == '\0' ) {
    xdgPath = QDir::homePath() + QLatin1Char( '/' ) + QLatin1String( defaultSubDir );
  } else if ( *env == '/' ) {
    xdgPath = QString::fromLocal8Bit( env );
  } else {
    xdgPath = QDir::homePath() + QLatin1Char( '/' ) + QString::fromLocal8Bit( env );
  }

  return xdgPath;
}

QStringList XdgBaseDirsSingleton::systemPathList( const char *variable, const char *defaultDirList )
{
  char *env = std::getenv( variable );

  QString xdgDirList;
  if ( env == 0 || *env == '\0' ) {
    xdgDirList = QLatin1String( defaultDirList );
  } else {
    xdgDirList = QString::fromLocal8Bit( env );
  }

  return xdgDirList.split( QLatin1Char( ':' ) );
}
