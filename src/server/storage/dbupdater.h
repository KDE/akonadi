/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QMap>
#include <QObject>
#include <QSqlDatabase>
#include <QStringList>

class QDomElement;
class DbUpdaterTest;

namespace Akonadi
{
namespace Server
{
/**
 * @short A helper class that contains an update set.
 */
class UpdateSet
{
public:
    using Map = QMap<int, UpdateSet>;

    UpdateSet()
        : version(-1)
        , abortOnFailure(false)
        , complex(false)
    {
    }

    int version;
    bool abortOnFailure;
    QStringList statements;
    bool complex;
};

/**
  Updates the database schema.
*/
class DbUpdater : public QObject
{
    Q_OBJECT

public:
    /**
     * Creates a new database updates.
     *
     * @param database The reference to the database.
     * @param filename The file containing the update descriptions.
     */
    DbUpdater(const QSqlDatabase &database, const QString &filename);

    /**
     * Starts the update process.
     * On success true is returned, false otherwise.
     */
    bool run();

private Q_SLOTS:
    bool complexUpdate_25();
    bool complexUpdate_36();

private:
    friend class ::DbUpdaterTest;

    bool updateApplicable(const QString &backends) const;
    QString buildRawSqlStatement(const QDomElement &element) const;

    bool parseUpdateSets(int, UpdateSet::Map &updates) const;

    QSqlDatabase m_database;
    QString m_filename;
};

} // namespace Server
} // namespace Akonadi
