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

#ifndef DBUSCONNECTIONPOOL_H
#define DBUSCONNECTIONPOOL_H

#include <QDBusConnection>

namespace Akonadi
{
namespace Server
{
namespace DBusConnectionPool
{

/**
 * Returns a new QDBusConnection for each thread, because QDBusConnection is
 * not thread-safe in Qt 4.
 *
 * FIXME: Remove in KF5
 */
QDBusConnection threadConnection();

}
}
}

#endif
