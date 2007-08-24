/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "searchprovidermanager.h"

#include "processcontrol.h"
#include "searchprovidermanageradaptor.h"
#include "xdgbasedirs.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>

Akonadi::SearchProviderManager::SearchProviderManager(QObject * parent) :
  QObject( parent )
{
  new SearchProviderManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/SearchProviderManager", this );

  mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBusConnection::sessionBus(), this );

  connect( QDBusConnection::sessionBus().interface(), SIGNAL( serviceOwnerChanged( const QString&, const QString&, const QString& ) ),
           this, SLOT( providerRegistered( const QString&, const QString&, const QString& ) ) );

  readProviderInfos();
  startProviders();
}

Akonadi::SearchProviderManager::~ SearchProviderManager()
{
  foreach ( ProviderInfo info, mProviderInfos ) {
    if ( info.interface )
      info.interface->quit();
  }
}

QStringList Akonadi::SearchProviderManager::providersForMimeType(const QString & mimeType) const
{
  QStringList result;
  foreach ( ProviderInfo info, mProviderInfos ) {
    if ( info.mimeTypes.contains( mimeType ) )
      result.append( info.identifier );
  }
  return result;
}

QStringList Akonadi::SearchProviderManager::providerInfoPathList()
{
  XdgBaseDirs baseDirs;
  return baseDirs.findAllResourceDirs( "data", QLatin1String( "akonadi/searchproviders" ) );
}

void Akonadi::SearchProviderManager::readProviderInfos()
{
  mProviderInfos.clear();

  QStringList pathList = providerInfoPathList();

  foreach ( QString path, pathList ) {
      QDir directory( path, "*.desktop" );
      readProviderInfos( directory );
  }
}

void Akonadi::SearchProviderManager::readProviderInfos( const QDir& directory )
{
  QStringList files = directory.entryList();
  for ( int i = 0; i < files.count(); ++i ) {
    const QString fileName = directory.absoluteFilePath( files[ i ] );

    QSettings file( fileName, QSettings::IniFormat );
    file.beginGroup( "Desktop Entry" );

    ProviderInfo info;
//     info.name = file.value( "Name" ).toString();
    info.mimeTypes = file.value( "X-Akonadi-MimeTypes" ).toStringList();
    info.exec = file.value( "Exec" ).toString();
    info.identifier= file.value( "X-Akonadi-Identifier" ).toString();
    info.controller = 0;
    info.interface = 0;

    file.endGroup();

    if ( info.identifier.isEmpty() ) {
      mTracer->error( QLatin1String( "SearchProviderManager::readProviderInfos" ),
                      QString( "Search provider desktop file '%1' contains empty identifier" ).arg( fileName ) );
      continue;
    }

    if ( mProviderInfos.contains( info.identifier ) ) {
      mTracer->error( QLatin1String( "SearchProviderManager::readPluginInfos" ),
                      QString( "Duplicated search provider identifier '%1' from file '%2'" ).arg( fileName, info.identifier ) );
      continue;
    }

    if ( info.exec.isEmpty() ) {
      mTracer->error( QLatin1String( "SearchProviderManager::readPluginInfos" ),
                      QString( "Search provider desktop file '%1' contains empty Exec entry" ).arg( fileName ) );
      continue;
    }

    mProviderInfos.insert( info.identifier, info );
  }
}

void Akonadi::SearchProviderManager::startProviders()
{
  foreach ( ProviderInfo info, mProviderInfos ) {
    if ( info.controller )
      continue;
    info.controller = new Akonadi::ProcessControl( this );
    info.controller->start( info.exec );
  }
}

void Akonadi::SearchProviderManager::providerRegistered(const QString & name, const QString &, const QString & newOwner)
{
  if ( !name.startsWith( "org.kde.Akonadi.SearchProvider." ) )
    return;

  const QString identifier = name.mid( 31 );

  if ( newOwner.isEmpty() ) { // interface was unregistered
    return;
  }

  delete mProviderInfos[ identifier ].interface;
  mProviderInfos[ identifier ].interface = 0;

  org::kde::Akonadi::SearchProvider *interface = new org::kde::Akonadi::SearchProvider(
      "org.kde.Akonadi.SearchProvider." + identifier, "/", QDBusConnection::sessionBus(), this );

  if ( !interface || !interface->isValid() ) {
    mTracer->error( QLatin1String( "SearchProviderManager::providerRegistered" ),
                    QString( "Cannot connect to provider instance with identifier '%1', error message: '%2'" )
                        .arg( identifier, interface ? interface->lastError().message() : "" ) );
    return;
  }

  interface->setObjectName( identifier );
  mProviderInfos[ identifier ].interface = interface;
}

#include "searchprovidermanager.moc"
