/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *   Copyright (C) 2013 by Volker Krause <vkrause@kde.org>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "schematypes.h"

#include <algorithm>

using namespace Akonadi::Server;

ColumnDescription::ColumnDescription()
    : size(-1)
    , allowNull(true)
    , isAutoIncrement(false)
    , isPrimaryKey(false)
    , isUnique(false)
    , isEnum(false)
    , onUpdate(Cascade)
    , onDelete(Cascade)
    , noUpdate(false)
{
}

IndexDescription::IndexDescription()
    : isUnique(false)
{
}

DataDescription::DataDescription()
{
}

TableDescription::TableDescription()
{
}

int TableDescription::primaryKeyColumnCount() const
{
    return std::count_if(columns.constBegin(), columns.constEnd(), [](const ColumnDescription & col) {
        return col.isPrimaryKey;
    }
                        );
}

RelationDescription::RelationDescription()
{
}

RelationTableDescription::RelationTableDescription(const RelationDescription &relation)
    : TableDescription()
{
    name = relation.firstTable + relation.secondTable + QStringLiteral("Relation");

    columns.reserve(2);
    ColumnDescription column;
    column.type = QStringLiteral("qint64");
    column.allowNull = false;
    column.isPrimaryKey = true;
    column.onUpdate = ColumnDescription::Cascade;
    column.onDelete = ColumnDescription::Cascade;
    column.name = relation.firstTable + QLatin1Char('_') + relation.firstColumn;
    column.refTable = relation.firstTable;
    column.refColumn = relation.firstColumn;
    columns.push_back(column);
    IndexDescription index;
    index.name = QStringLiteral("%1Index").arg(column.name);
    index.columns = QStringList{column.name};
    index.isUnique = false;
    indexes.push_back(index);

    column.name = relation.secondTable + QLatin1Char('_') + relation.secondColumn;
    column.refTable = relation.secondTable;
    column.refColumn = relation.secondColumn;
    columns.push_back(column);
    index.name = QStringLiteral("%1Index").arg(column.name);
    index.columns = QStringList{column.name};
    indexes.push_back(index);

    indexes += relation.indexes;
}
