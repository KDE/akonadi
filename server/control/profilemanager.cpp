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

#include <QtCore/QDir>
#include <QtCore/QSettings>

#include "profilemanager.h"


ProfileManager::ProfileManager( QObject *parent )
  : QObject( parent )
{
  load();
}

ProfileManager::~ProfileManager()
{
  save();
}

void ProfileManager::load()
{
  QSettings settings( profilePath(), QSettings::IniFormat );

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
  QSettings settings( profilePath(), QSettings::IniFormat );
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
  if ( mProfiles.contains( identifier ) )
    return false;

  mProfiles.insert( identifier, QStringList() );

  save();

  return true;
}

void ProfileManager::removeProfile( const QString &identifier )
{
  mProfiles.remove( identifier );

  save();
}

bool ProfileManager::profileAddAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  if ( !mProfiles.contains( profileIdentifier ) )
    return false;

  if ( mProfiles[ profileIdentifier ].contains( agentIdentifier ) )
    return false;

  mProfiles[ profileIdentifier ].append( agentIdentifier );

  save();

  return true;
}

bool ProfileManager::profileRemoveAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  if ( !mProfiles.contains( profileIdentifier ) )
    return false;

  if ( !mProfiles[ profileIdentifier ].contains( agentIdentifier ) )
    return false;

  mProfiles[ profileIdentifier ].removeAll( agentIdentifier );

  save();

  return true;
}

QStringList ProfileManager::profileAgents( const QString &identifier ) const
{
  if ( !mProfiles.contains( identifier ) )
    return QStringList();

  return mProfiles[ identifier ];
}

QString ProfileManager::profilePath() const
{
  const QString homePath = QDir::homePath();
  const QString akonadiHomeDir = QString( "%1/%2" ).arg( homePath, ".akonadi" );

  if ( !QDir( akonadiHomeDir ).exists() ) {
    QDir dir;
    dir.mkdir( akonadiHomeDir );
  }

  return akonadiHomeDir + "/profilesrc";
}
