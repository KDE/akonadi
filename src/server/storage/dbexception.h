/*
    SPDX-FileCopyrightText: 2012 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "exception.h"

class QSqlQuery;

namespace Akonadi
{
namespace Server
{
/** Exception for reporting SQL errors. */
class DbException : public Exception
{
public:
    explicit DbException(const QSqlQuery &query, const char *what = nullptr);
    const char *type() const throw() override;
};

class DbDeadlockException : public DbException
{
public:
    explicit DbDeadlockException(const QSqlQuery &query);
};

} // namespace Server
} // namespace Akonadi
