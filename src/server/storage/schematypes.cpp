/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *   SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#include "schematypes.h"

#include <algorithm>

using namespace Akonadi::Server;

int TableDescription::primaryKeyColumnCount() const
{
    return std::count_if(columns.constBegin(), columns.constEnd(), [](const ColumnDescription &col) {
        return col.isPrimaryKey;
    });
}

RelationTableDescription::RelationTableDescription(const RelationDescription &relation)
    : TableDescription()
{
    name = relation.firstTable + relation.secondTable + QStringLiteral("Relation");
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_GCC("-Wmissing-designated-field-initializers")
    QT_WARNING_DISABLE_CLANG("-Wmissing-designated-field-initializers")
    columns = {ColumnDescription{.name = QStringLiteral("%1_%2").arg(relation.firstTable, relation.firstColumn),
                                 .type = QStringLiteral("qint64"),
                                 .allowNull = false,
                                 .isPrimaryKey = true,
                                 .refTable = relation.firstTable,
                                 .refColumn = relation.firstColumn},
               ColumnDescription{.name = QStringLiteral("%1_%2").arg(relation.secondTable, relation.secondColumn),
                                 .type = QStringLiteral("qint64"),
                                 .allowNull = false,
                                 .isPrimaryKey = true,
                                 .refTable = relation.secondTable,
                                 .refColumn = relation.secondColumn}};
    indexes = {IndexDescription{.name = QStringLiteral("%1Index").arg(columns[0].name), .columns = {columns[0].name}, .isUnique = false},
               IndexDescription{.name = QStringLiteral("%1Index").arg(columns[1].name), .columns = {columns[1].name}, .isUnique = false}};
    indexes += relation.indexes;
    QT_WARNING_POP
}
