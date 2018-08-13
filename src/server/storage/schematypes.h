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

#ifndef SCHEMATYPES_H
#define SCHEMATYPES_H

#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

namespace Akonadi
{
namespace Server
{

/**
  * @short A helper class that describes a column of a table for the DbInitializer
  */
class ColumnDescription
{
public:
    ColumnDescription();

    enum ReferentialAction {
        Cascade,
        Restrict,
        SetNull
    };

    QString name;
    QString type;
    int size;
    bool allowNull;
    bool isAutoIncrement;
    bool isPrimaryKey;
    bool isUnique;
    bool isEnum;
    QString refTable;
    QString refColumn;
    QString defaultValue;
    ReferentialAction onUpdate;
    ReferentialAction onDelete;
    bool noUpdate;

    QMap<QString, int> enumValueMap;
};

/**
  * @short A helper class that describes indexes of a table for the DbInitializer
  */
class IndexDescription
{
public:
    IndexDescription();

    QString name;
    QStringList columns;
    bool isUnique;
    QString sort;
};

/**
  * @short A helper class that describes the predefined data of a table for the DbInitializer
  */
class DataDescription
{
public:
    DataDescription();

    /**
      * Key contains the column name, value the data.
      */
    QMap<QString, QString> data;
};

/**
  * @short A helper class that describes a table for the DbInitializer
  */
class TableDescription
{
public:
    TableDescription();
    int primaryKeyColumnCount() const;

    QString name;
    QVector<ColumnDescription> columns;
    QVector<IndexDescription> indexes;
    QVector<DataDescription> data;
};

/**
  * @short A helper class that describes the relation between two tables for the DbInitializer
  */
class RelationDescription
{
public:
    RelationDescription();

    QString firstTable;
    QString firstColumn;
    QString secondTable;
    QString secondColumn;
    QVector<IndexDescription> indexes;
};

/**
 * @short TableDescription constructed based on RelationDescription
 */
class RelationTableDescription : public TableDescription
{
public:
    explicit RelationTableDescription(const RelationDescription &relation);
};

} // namespace Server
} // namespace Akonadi

#endif
