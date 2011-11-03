/*
    Copyright (c) 2011 Volker Krause <vkrause@kde.org>

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

#include "akdbus.h"
#include "akapplication.h"

#include <libs/protocol_p.h>

#include <QString>
#include <QStringBuilder>

static QString makeServiceName( const char* base )
{
  if (AkApplication::instanceIdentifier().isEmpty())
    return QLatin1String(base);
  return QLatin1String(base) % QLatin1Literal(".") % AkApplication::instanceIdentifier();
}


QString AkDBus::serviceName(AkDBus::ServiceType serviceType)
{
  switch(serviceType) {
    case Server: return makeServiceName(AKONADI_DBUS_SERVER_SERVICE);
    case Control: return makeServiceName(AKONADI_DBUS_CONTROL_SERVICE);
    case ControlLock: return makeServiceName(AKONADI_DBUS_CONTROL_SERVICE_LOCK);
    case AgentServer: return makeServiceName(AKONADI_DBUS_AGENTSERVER_SERVICE);
    case StorageJanitor: return makeServiceName(AKONADI_DBUS_STORAGEJANITOR_SERVICE);
  }
  Q_ASSERT(!"WTF?");
  return QString();
}
