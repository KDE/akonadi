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

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>

#include <cstdlib>

static QStringList alternateExecPaths( const QString& path )
{
  QStringList pathList;

  pathList << path;

#if defined(Q_OS_WIN) //krazy:exclude=cpp
  pathList << path + QLatin1String( ".exe" );
#elif defined(Q_OS_MAC) //krazy:exclude=cpp
  pathList << path + QLatin1String( ".app/Contents/MacOS/" ) + path.section( QLatin1Char( '/' ), -1 );
#endif

  return pathList;
}

static QStringList splitPathList( const QString &pathList )
{
#if defined(Q_OS_WIN) //krazy:exclude=cpp
  return pathList.split( QLatin1Char( ';' ) );
#else
  return pathList.split( QLatin1Char( ':' ) );
#endif
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

Q_GLOBAL_STATIC(XdgBaseDirsSingleton, instance)

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

      // fallback for users with KDE in a different prefix and not correctly set up XDG_DATA_DIRS, hi David ;-)
      QProcess proc;
      // ### should probably rather be --path xdg-something
      const QStringList args = QStringList() << QLatin1String( "--prefix" );
      proc.start( QLatin1String( "kde4-config" ), args );
      if ( proc.waitForStarted() && proc.waitForFinished() && proc.exitCode() == 0 ) {
        proc.setReadChannel( QProcess::StandardOutput );
        foreach ( const QString &basePath, splitPathList( QString::fromLocal8Bit( proc.readLine().trimmed() ) ) ) {
          const QString path = basePath + QDir::separator() + QLatin1String( "share" );
          if ( !dataDirs.contains( path ) )
            dataDirs << path;
        }
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

  QStringList::const_iterator it    = pathList.constBegin();
  QStringList::const_iterator endIt = pathList.constEnd();
  for ( ; it != endIt; ++it ) {
    fileInfo = QFileInfo( *it + QLatin1Char('/' ) + relPath );
    if ( fileInfo.exists() && fileInfo.isFile() && fileInfo.isReadable() ) {
      return fileInfo.absoluteFilePath();
    }
  }

  return QString();
}

QString XdgBaseDirs::findExecutableFile( const QString &relPath, const QStringList &searchPath )
{
  if ( instance()->mExecutableDirs.isEmpty() ) {
    QStringList executableDirs = instance()->systemPathList( "PATH", "/usr/local/bin:/usr/bin" );

    const QString prefixExecutableDir = QLatin1String( AKONADIPREFIX "/bin" );
    if ( !executableDirs.contains( prefixExecutableDir ) ) {
      executableDirs << prefixExecutableDir;
    }

    executableDirs += searchPath;

    instance()->mExecutableDirs = executableDirs;
  }

  QStringList::const_iterator pathIt    = instance()->mExecutableDirs.constBegin();
  QStringList::const_iterator pathEndIt = instance()->mExecutableDirs.constEnd();
  for ( ; pathIt != pathEndIt; ++pathIt ) {
    QStringList fullPathList = alternateExecPaths(*pathIt + QLatin1Char( '/' ) + relPath );

    QStringList::const_iterator it    = fullPathList.constBegin();
    QStringList::const_iterator endIt = fullPathList.constEnd();
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

  QStringList::const_iterator it    = pathList.constBegin();
  QStringList::const_iterator endIt = pathList.constEnd();
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

  QStringList::const_iterator it    = pathList.constBegin();
  QStringList::const_iterator endIt = pathList.constEnd();
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
  const QByteArray env = qgetenv( variable );

  QString xdgPath;
  if ( env.isEmpty() ) {
    xdgPath = QDir::homePath() + QLatin1Char( '/' ) + QLatin1String( defaultSubDir );
#if defined(Q_OS_WIN) //krazy:exclude=cpp
  } else if ( QDir::isAbsolutePath( QString::fromLocal8Bit( env ) ) ) {
#else
  } else if ( env.startsWith( '/' ) ) {
#endif
    xdgPath = QString::fromLocal8Bit( env );
  } else {
    xdgPath = QDir::homePath() + QLatin1Char( '/' ) + QString::fromLocal8Bit( env );
  }

  return xdgPath;
}

QStringList XdgBaseDirsSingleton::systemPathList( const char *variable, const char *defaultDirList )
{
  const QByteArray env = qgetenv( variable );

  QString xdgDirList;
  if ( env.isEmpty() ) {
    xdgDirList = QLatin1String( defaultDirList );
  } else {
    xdgDirList = QString::fromLocal8Bit( env );
  }

  return splitPathList( xdgDirList );
}
