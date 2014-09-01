/***************************************************************************
 *   Copyright (C) 2013 by Volker Krause <vkrause@kde.org>                 *
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

#ifndef CLIENTCAPABILITYAGGREGATOR_H
#define CLIENTCAPABILITYAGGREGATOR_H

#include "clientcapabilities.h"

namespace Akonadi {
namespace Server {

/** Aggregates client capabilities of all active sessions. */
namespace ClientCapabilityAggregator {
/** Register capabilities of a new session. */
void addSession(const ClientCapabilities &capabilities);
/** Unregister capabilities of a new session. */
void removeSession(const ClientCapabilities &capabilities);

/** Minimum required notification message version. */
int minimumNotificationMessageVersion();

/** Maximum required notification message version */
int maximumNotificationMessageVersion();
}

} // namespace Server
} // namespace Akonadi

#endif // CLIENTCAPABILITYAGGREGATOR_H
