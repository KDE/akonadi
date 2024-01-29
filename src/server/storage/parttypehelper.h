/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "exception.h"
#include "query.h"

namespace Akonadi
{
namespace Server
{
class PartType;

AKONADI_EXCEPTION_MAKE_INSTANCE(PartTypeException);

/**
 * Methods for dealing with the PartType table.
 */
namespace PartTypeHelper
{
/**
 * Retrieve (or create) PartType for the given fully qualified name.
 * @param fqName Fully qualified name (NS:NAME).
 * @throws PartTypeException
 */
PartType fromFqName(const QString &fqName);

/**
 * Convenience overload of the above.
 */
PartType fromFqName(const QByteArray &fqName);

/**
 * Retrieve (or create) PartType for the given namespace and type name.
 * @param ns Namespace
 * @param typeName Part type name.
 * @throws PartTypeException
 */
PartType fromFqName(const QString &ns, const QString &typeName);

/**
 * Returns a query condition that matches the given part.
 * @param fqName fully-qualified part type name
 * @throws PartTypeException
 */
Query::Condition conditionFromFqName(const QString &fqName);

/**
 * Returns a query condition that matches the given part type list.
 * @param fqNames fully qualified part type name list
 * @throws PartTypeException
 */
Query::Condition conditionFromFqNames(const QStringList &fqNames);

/**
 * Convenience overload for the above.
 */
template<template<typename> class T>
Query::Condition conditionFromFqNames(const T<QByteArray> &fqNames)
{
    Query::Condition c;
    c.setSubQueryMode(Query::Or);
    for (const QByteArray &fqName : fqNames) {
        c.addCondition(conditionFromFqName(QLatin1StringView(fqName)));
    }
    return c;
}

/**
 * Parses a fully qualified part type name into namespace/name.
 * @param fqName fully-qualified part type name
 * @throws PartTypeException if @p fqName does not match the NS:NAME schema
 * @internal
 */
std::pair<QString, QString> parseFqName(const QString &fqName);

/**
 * Returns full part name
 */
QString fullName(const PartType &type);

} // namespace PartTypeHelper

} // namespace Server
} // namespace Akonadi
