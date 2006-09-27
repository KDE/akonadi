/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include "profilemanager.h"
#include "profilemanagerinterface.h"

using namespace Akonadi;

class ProfileManager::Private
{
  public:
    org::kde::Akonadi::ProfileManager *mManager;
};

ProfileManager::ProfileManager( QObject *parent )
  : QObject( parent ), d( new Private )
{
  d->mManager = new org::kde::Akonadi::ProfileManager( QLatin1String( "org.kde.Akonadi.Control" ),
                                                       QLatin1String( "/ProfileManager" ),
                                                       QDBusConnection::sessionBus(), this );

  connect( d->mManager, SIGNAL( profileAdded( const QString& ) ),
           this, SIGNAL( profileAdded( const QString& ) ) );
  connect( d->mManager, SIGNAL( profileRemoved( const QString& ) ),
           this, SIGNAL( profileRemoved( const QString& ) ) );
  connect( d->mManager, SIGNAL( profileAgentAdded( const QString&, const QString& ) ),
           this, SIGNAL( profileAgentAdded( const QString&, const QString& ) ) );
  connect( d->mManager, SIGNAL( profileAgentRemoved( const QString&, const QString& ) ),
           this, SIGNAL( profileAgentRemoved( const QString&, const QString& ) ) );
}

ProfileManager::~ProfileManager()
{
  delete d;
}

QStringList ProfileManager::profiles() const
{
  return d->mManager->profiles();
}

bool ProfileManager::createProfile( const QString &identifier )
{
  return d->mManager->createProfile( identifier );
}

void ProfileManager::removeProfile( const QString &identifier )
{
  d->mManager->removeProfile( identifier );
}

bool ProfileManager::profileAddAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  return d->mManager->profileAddAgent( profileIdentifier, agentIdentifier );
}

bool ProfileManager::profileRemoveAgent( const QString &profileIdentifier, const QString &agentIdentifier )
{
  return d->mManager->profileRemoveAgent( profileIdentifier, agentIdentifier );
}

QStringList ProfileManager::profileAgents( const QString &identifier ) const
{
  return d->mManager->profileAgents( identifier );
}

#include "profilemanager.moc"
