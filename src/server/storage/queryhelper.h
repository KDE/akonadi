/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_QUERYHELPER_H
#define AKONADI_QUERYHELPER_H

class QString;

namespace Akonadi
{

class ImapSet;

namespace Server
{

class QueryBuilder;

/**
  Helper methods for common query tasks.
*/
namespace QueryHelper
{
/**
  Add conditions to @p qb for the given uid set @p set applied to @p column.
*/
void setToQuery(const ImapSet &set, const QString &column, QueryBuilder &qb);

} // namespace QueryHelper

} // namespace Server
} // namespace Akonadi

#endif
