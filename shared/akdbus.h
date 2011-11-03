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

#ifndef AKDBUS_H
#define AKDBUS_H

class QString;

/**
 * Helper methods for obtaining D-Bus identifiers.
 * This should be used instead of hardcoded identifiers or constants to support multi-instance namespacing
 * @since 1.7
 */
namespace AkDBus
{
  /** D-Bus service types used by the Akonadi server processes. */
  enum ServiceType {
    Server,
    Control,
    ControlLock,
    AgentServer,
    StorageJanitor
  };
  
  /**
   * Returns the service name for the given @p serviceType.
   */
  QString serviceName( ServiceType serviceType );  
}

#endif
