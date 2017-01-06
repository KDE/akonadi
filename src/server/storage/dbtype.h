/*
    Copyright (c) 2012 Volker Krause <vkrause@kde.org>

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

#ifndef DBTYPE_H
#define DBTYPE_H

#include <QSqlDatabase>

namespace Akonadi
{
namespace Server
{

/** Helper methods for checking the database system we are dealing with. */
namespace DbType
{

/** Supported database types. */
enum Type {
    Unknown,
    Sqlite,
    MySQL,
    PostgreSQL
};

/** Returns the type of the given database object. */
Type type(const QSqlDatabase &db);

/** Returns the type for the given driver name. */
Type typeForDriverName(const QString &driverName);

/** Returns true when using QSQLITE driver shipped with Qt, FALSE otherwise */
bool isSystemSQLite(const QSqlDatabase &db);

} // namespace DbType
} // namespace Server
} // namespace Akonadi

#endif // DBTYPE_H
