/*
 * Copyright (C) 2010 Sebastian Trueg <trueg@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _DBUS_CONNECTION_POOL_H_
#define _DBUS_CONNECTION_POOL_H_

#include "akonadicore_export.h"

#include <QtDBus/QDBusConnection>

/**
  NOTE: This method is for use in libakonadi-kde and targets *under*
        kdepimlibs/akonadi. In kdelibs 4.6 there will be a slightly more generic
        variant available. We need this method as long as we depend on kdelibs
        4.5 which doesn't have this method.

*/

namespace Akonadi {

namespace DBusConnectionPool {
/**
 * The DBusConnectionPool works around the problem
 * of QDBusConnection not being thread-safe. As soon as that
 * has been fixed (either directly in libdbus or with a work-
 * around in Qt) this method can be dropped in favor of
 * QDBusConnection::sessionBus().
 */
AKONADICORE_EXPORT QDBusConnection threadConnection();
} // DBusConnectionPool

} // Akonadi

#endif
