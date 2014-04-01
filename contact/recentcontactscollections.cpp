/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "recentcontactscollections_p.h"

#include "recentcontactscollectionssettings.h"

#include <KGlobal>

#include "akonadi/agentinstance.h"
#include "akonadi/servermanager.h"

using namespace Akonadi;

class Akonadi::RecentContactsCollectionsPrivate
{
  public:
    RecentContactsCollectionsPrivate();
    ~RecentContactsCollectionsPrivate();

    RecentContactsCollections *mInstance;
};

typedef RecentContactsCollectionsSettings Settings;

Q_GLOBAL_STATIC( RecentContactsCollectionsPrivate, sInstance )

static const QByteArray sRecentContactsType = "recent-contacts";

RecentContactsCollectionsPrivate::RecentContactsCollectionsPrivate()
  : mInstance( new RecentContactsCollections( this ) )
{
}

RecentContactsCollectionsPrivate::~RecentContactsCollectionsPrivate()
{
  delete mInstance;
}

static KCoreConfigSkeleton *getConfig( const QString &filename)
{
  Settings::instance( ServerManager::addNamespace( filename ) );
  return Settings::self();
}

RecentContactsCollections::RecentContactsCollections( RecentContactsCollectionsPrivate *dd )
  : SpecialCollections( getConfig(QStringLiteral("recentcontactscollectionsrc")) ),
    d( dd )
{
  Q_UNUSED(d); // d isn't used yet
}

RecentContactsCollections *RecentContactsCollections::self()
{
  return sInstance->mInstance;
}

bool RecentContactsCollections::hasCollection( const AgentInstance &instance ) const
{
  return SpecialCollections::hasCollection( sRecentContactsType, instance );
}

Collection RecentContactsCollections::collection( const AgentInstance &instance ) const
{
  return SpecialCollections::collection( sRecentContactsType, instance );
}

bool RecentContactsCollections::registerCollection( const Collection &collection )
{
  return SpecialCollections::registerCollection( sRecentContactsType, collection );
}

bool RecentContactsCollections::hasDefaultCollection() const
{
  return SpecialCollections::hasDefaultCollection( sRecentContactsType );
}

Collection RecentContactsCollections::defaultCollection() const
{
  return SpecialCollections::defaultCollection( sRecentContactsType );
}

#include "moc_recentcontactscollections_p.cpp"
