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

#include "parttypehelper.h"

#include "selectquerybuilder.h"
#include "entities.h"

#include <QStringList>

using namespace Akonadi::Server;

QPair< QString, QString > PartTypeHelper::parseFqName(const QString &fqName)
{
    const QStringList name = fqName.split(QLatin1Char(':'), QString::SkipEmptyParts);
    if (name.size() != 2) {
        throw PartTypeException("Invalid part type name.");
    }
    return qMakePair(name.first(), name.last());
}

PartType PartTypeHelper::fromFqName(const QString &fqName)
{
    const QPair<QString, QString> p = parseFqName(fqName);
    return fromFqName(p.first, p.second);
}

PartType PartTypeHelper::fromFqName(const QByteArray &fqName)
{
    return fromFqName(QLatin1String(fqName));
}

PartType PartTypeHelper::fromFqName(const QString &ns, const QString &name)
{
    const PartType partType = PartType::retrieveByFQNameOrCreate(ns, name);
    if (!partType.isValid()) {
        throw PartTypeException("Failed to append part type");
    }
    return partType;
}

Query::Condition PartTypeHelper::conditionFromFqName(const QString &fqName)
{
    const QPair<QString, QString> p = parseFqName(fqName);
    Query::Condition c;
    c.setSubQueryMode(Query::And);
    c.addValueCondition(PartType::nsFullColumnName(), Query::Equals, p.first);
    c.addValueCondition(PartType::nameFullColumnName(), Query::Equals, p.second);
    return c;
}

Query::Condition PartTypeHelper::conditionFromFqNames(const QStringList &fqNames)
{
    Query::Condition c;
    c.setSubQueryMode(Query::Or);
    for (const QString &fqName : fqNames) {
        c.addCondition(conditionFromFqName(fqName));
    }
    return c;
}

QString PartTypeHelper::fullName(const PartType &type)
{
    return type.ns() + QLatin1String(":") + type.name();
}
