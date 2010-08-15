/*
    This file is part of akonadiresources.

    Copyright (c) 2006 Till Adam <adam@kde.org>
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "agentfactory.h"

#include <klocale.h>

using namespace Akonadi;

AgentFactoryBase::AgentFactoryBase(const char* catalogName, QObject* parent):
  QObject( parent )
{
  if ( !KGlobal::hasMainComponent() )
    new KComponentData( "AkoanadiAgentServer", "libakonadi", KComponentData::RegisterAsMainComponent );

  KGlobal::locale()->insertCatalog( QLatin1String( catalogName ) );
}

#include "agentfactory.moc"
