/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

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
    PostgreSQL,
};

/** Returns the type of the given database object. */
Type type(const QSqlDatabase &db);

/** Returns the type for the given driver name. */
Type typeForDriverName(const QString &driverName);

} // namespace DbType
} // namespace Server
} // namespace Akonadi
