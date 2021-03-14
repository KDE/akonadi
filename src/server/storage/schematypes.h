/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *   SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
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
        SetNull,
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
