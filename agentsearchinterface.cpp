/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "agentsearchinterface.h"
#include "agentsearchinterface_p.h"
#include "collection.h"
#include "dbusconnectionpool.h"
#include "searchadaptor.h"

using namespace Akonadi;

AgentSearchInterfacePrivate::AgentSearchInterfacePrivate( AgentSearchInterface* qq ) :
 q( qq )
{
  new Akonadi__SearchAdaptor( this );
  DBusConnectionPool::threadConnection().registerObject( QLatin1String( "/Search" ),
                                                         this, QDBusConnection::ExportAdaptors );
}

void AgentSearchInterfacePrivate::addSearch( const QString &query, const QString &queryLanguage, quint64 resultCollectionId )
{
  q->addSearch( query, queryLanguage, Collection( resultCollectionId ) );
}

void AgentSearchInterfacePrivate::removeSearch( quint64 resultCollectionId )
{
  q->removeSearch( Collection( resultCollectionId ) );
}

AgentSearchInterface::AgentSearchInterface() :
  d( new AgentSearchInterfacePrivate( this ) )
{
}

AgentSearchInterface::~AgentSearchInterface()
{
  delete d;
}

#include "agentsearchinterface_p.moc"
