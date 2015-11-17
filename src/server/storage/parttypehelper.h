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

#ifndef PARTTYPEHELPER_H
#define PARTTYPEHELPER_H

#include "exception.h"
#include "query.h"

namespace Akonadi {
namespace Server {

class PartType;

AKONADI_EXCEPTION_MAKE_INSTANCE(PartTypeException);

/**
 * Methods for dealing with the PartType table.
 */
namespace PartTypeHelper {
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
    Q_FOREACH (const QByteArray &fqName, fqNames) {
        c.addCondition(conditionFromFqName(QLatin1String(fqName)));
    }
    return c;
}

/**
 * Parses a fully qualified part type name into namespace/name.
 * @param fqName fully-qualified part type name
 * @throws PartTypeException if @p fqName does not match the NS:NAME schema
 * @internal
 */
QPair<QString, QString> parseFqName(const QString &fqName);

/**
 * Returns full part name
 */
QString fullName(const PartType &type);

} // namespace PartTypeHelper

} // namespace Server
} // namespace Akonadi

#endif
