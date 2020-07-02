/*
    SPDX-FileCopyrightText: 2011 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "parttypehelper.h"

#include "selectquerybuilder.h"
#include "entities.h"

#include <QStringList>

#include <tuple>

using namespace Akonadi::Server;

std::pair<QString, QString> PartTypeHelper::parseFqName(const QString &fqName)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    const QStringList name = fqName.split(QLatin1Char(':'), QString::SkipEmptyParts);
#else
    const QStringList name = fqName.split(QLatin1Char(':'), Qt::SkipEmptyParts);
#endif
    if (name.size() != 2) {
        throw PartTypeException("Invalid part type name.");
    }
    return {name.first(), name.last()};
}

PartType PartTypeHelper::fromFqName(const QString &fqName)
{
    return std::apply(qOverload<const QString &, const QString &>(PartTypeHelper::fromFqName), parseFqName(fqName));
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
    const auto [ns, name] = parseFqName(fqName);
    Query::Condition c;
    c.setSubQueryMode(Query::And);
    c.addValueCondition(PartType::nsFullColumnName(), Query::Equals, ns);
    c.addValueCondition(PartType::nameFullColumnName(), Query::Equals, name);
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
