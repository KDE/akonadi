/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "profilemanager.h"

#include "profilemanageradaptor.h"
#include <akonadi/private/xdgbasedirs_p.h>

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>

ProfileManager::ProfileManager( QObject *parent )
  : QObject( parent )
{
  new ProfileManagerAdaptor( this );
  QDBusConnection::sessionBus().registerObject( "/ProfileManager", this );

  mTracer = new org::kde::Akonadi::Tracer( "org.kde.Akonadi", "/tracing", QDBusConnection::sessionBus(), this );

  load();
}

ProfileManager::~ProfileManager()
{
  save();
}

void ProfileManager::load()
{
  QSettings settings( profilePath( false ), QSettings::IniFormat );

  const QStringList profiles = settings.childGroups();
  for ( int i = 0; i < profiles.count(); ++i ) {
    settings.beginGroup( profiles[ i ] );
    const QString identifier = settings.value( "identifier" ).toString();
    const QStringList agentIdentifiers = settings.value( "agentIdentifiers" ).toStringList();
    settings.endGroup();

    mProfiles.insert( identifier, agentIdentifiers );
  }
}

void ProfileManager::save()
{
  QSettings settings( profilePath( true ), QSettings::IniFormat );
  settings.clear();

  QMapIterator<QString, QStringList> it( mProfiles );
  while ( it.hasNext() ) {
    it.next();

    settings.beginGroup( it.key() );
    settings.setValue( "identifier", it.key() );
    settings.setValue( "agentIdentifiers", it.value() );
    settings.endGroup();
  }

  settings.sync();
}

QStringList ProfileManager::profiles() const
{
  return mProfiles.keys();
}

bool ProfileManager::createProfile( const QString &identifier )
{
  if ( mProfiles.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "ProfileManager::createProfile" ),
                     QString( "Profile with identifier '%1' already exists" ).arg( identifier ) );
    return false;
  }

  mProfiles.insert( identifier, QStringList() );

  save();

  emit profileAdded( identifier );

  return true;
}

void ProfileManager::removeProfile( const QString &identifier )
{
  mProfiles.remove( identifier );

  save();

  emit profileRemoved( identifier );
}

bool ProfileManager::profileAddAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  if ( !mProfiles.contains( profileIdentifier ) ) {
    mTracer->warning( QLatin1String( "ProfileManager::profileAddAgent" ),
                     QString( "Profile '%1' does not exists" ).arg( profileIdentifier ) );
    return false;
  }

  if ( mProfiles[ profileIdentifier ].contains( agentIdentifier ) ) {
    mTracer->warning( QLatin1String( "ProfileManager::profileAddAgent" ),
                     QString( "Profile '%1' contains agent '%1' already" ).arg( profileIdentifier, agentIdentifier ) );
    return false;
  }

  mProfiles[ profileIdentifier ].append( agentIdentifier );

  save();

  emit profileAgentAdded( profileIdentifier, agentIdentifier );

  return true;
}

bool ProfileManager::profileRemoveAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  if ( !mProfiles.contains( profileIdentifier ) ) {
    mTracer->warning( QLatin1String( "ProfileManager::profileRemoveAgent" ),
                     QString( "Profile '%1' does not exists" ).arg( profileIdentifier ) );
    return false;
  }

  if ( !mProfiles[ profileIdentifier ].contains( agentIdentifier ) ) {
    mTracer->warning( QLatin1String( "ProfileManager::profileRemoveAgent" ),
                     QString( "Profile '%1' does not contain agent '%1'" ).arg( profileIdentifier, agentIdentifier ) );
    return false;
  }

  mProfiles[ profileIdentifier ].removeAll( agentIdentifier );

  save();

  emit profileAgentRemoved( profileIdentifier, agentIdentifier );

  return true;
}

QStringList ProfileManager::profileAgents( const QString &identifier ) const
{
  if ( !mProfiles.contains( identifier ) ) {
    mTracer->warning( QLatin1String( "ProfileManager::profileAgents" ),
                     QString( "Profile '%1' does not exist" ).arg( identifier ) );
    return QStringList();
  }

  return mProfiles[ identifier ];
}

QString ProfileManager::profilePath( bool writeable ) const
{
  const QString configFile =
    Akonadi::XdgBaseDirs::findResourceFile( "config", QLatin1String( "akonadi/profilesrc" ) );

  if ( !writeable && !configFile.isEmpty() )
    return configFile;

  const QString configDir = Akonadi::XdgBaseDirs::saveDir( "config", "akonadi" );

  return configDir + QLatin1String( "/profilesrc" );
}

#include "profilemanager.moc"
