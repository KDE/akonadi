/***************************************************************************
 *   SPDX-FileCopyrightText: 2006 Tobias Koenig <tokoe@kde.org>            *
 *   SPDX-FileCopyrightText: 2013 Volker Krause <vkrause@kde.org>          *
 *                                                                         *
 *   SPDX-License-Identifier: LGPL-2.0-or-later                            *
 ***************************************************************************/

#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <QStringList>

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
    enum ReferentialAction {
        Cascade,
        Restrict,
        SetNull,
    };

    QString name;
    QString type;
    int size = -1;
    bool allowNull = true;
    bool isAutoIncrement = false;
    bool isPrimaryKey = false;
    bool isUnique = false;
    bool isEnum = false;
    QString refTable;
    QString refColumn;
    QString defaultValue;
    ReferentialAction onUpdate = Cascade;
    ReferentialAction onDelete = Cascade;
    bool noUpdate = false;

    QMap<QString, int> enumValueMap;
};

/**
 * @short A helper class that describes indexes of a table for the DbInitializer
 */
class IndexDescription
{
public:
    QString name;
    QStringList columns;
    bool isUnique = false;
    QString sort;
};

/**
 * @short A helper class that describes the predefined data of a table for the DbInitializer
 */
class DataDescription
{
public:
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
    int primaryKeyColumnCount() const;

    QString name;
    QList<ColumnDescription> columns;
    QList<IndexDescription> indexes;
    QList<DataDescription> data;
};

/**
 * @short A helper class that describes the relation between two tables for the DbInitializer
 */
class RelationDescription
{
public:
    QString firstTable;
    QString firstColumn;
    QString secondTable;
    QString secondColumn;
    QList<IndexDescription> indexes;
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

Q_DECLARE_TYPEINFO(Akonadi::Server::ColumnDescription, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Akonadi::Server::IndexDescription, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Akonadi::Server::DataDescription, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Akonadi::Server::TableDescription, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Akonadi::Server::RelationDescription, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(Akonadi::Server::RelationTableDescription, Q_RELOCATABLE_TYPE);
