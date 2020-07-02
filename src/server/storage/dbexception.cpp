/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dbexception.h"

#include <QSqlError>
#include <QSqlQuery>

using namespace Akonadi::Server;

DbException::DbException(const QSqlQuery &query, const char *what)
    : Exception(what)
{
    mWhat += "\nSql error: " + query.lastError().text().toUtf8();
    mWhat += "\nQuery: " + query.lastQuery().toUtf8();
}

const char *DbException::type() const throw()
{
    return "Database Exception";
}

DbDeadlockException::DbDeadlockException(const QSqlQuery &query)
    : DbException(query, "Database deadlock, unsuccessful after multiple retries")
{
}
