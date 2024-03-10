/*
 * SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 */

#pragma once

#include <QDateTime>
#include <QTimeZone>
#include <QVariant>

#include "storage/datastore.h"
#include "storage/dbtype.h"

namespace Akonadi
{
namespace Server
{
namespace Utils
{
/**
 * Converts a QVariant to a QString depending on its internal type.
 */
static inline QString variantToString(const QVariant &variant)
{
    if (variant.typeId() == QMetaType::QString) {
        return variant.toString();
    } else if (variant.typeId() == QMetaType::QByteArray) {
        return QString::fromUtf8(variant.toByteArray());
    } else {
        qWarning("Unable to convert variant of type %s to QString", variant.typeName());
        Q_ASSERT(false);
        return QString();
    }
}

/**
 * Converts a QVariant to a QByteArray depending on its internal type.
 */
static inline QByteArray variantToByteArray(const QVariant &variant)
{
    if (variant.typeId() == QMetaType::QString) {
        return variant.toString().toUtf8();
    } else if (variant.typeId() == QMetaType::QByteArray) {
        return variant.toByteArray();
    } else {
        qWarning("Unable to convert variant of type %s to QByteArray", variant.typeName());
        Q_ASSERT(false);
        return QByteArray();
    }
}

/**
 * Converts QDateTime to a QVariant suitable for storing in the database.
 *
 * It should come as no surprise that different database engines and their QtSql drivers have each a different
 * approach to time and timezones.
 * Here we make sure that whatever QDateTime we have, it will be stored in the database in a way
 * that matches its (and its driver's) perception of timezones.
 *
 * Check the dbdatetimetest for more.
 */
static inline QVariant dateTimeToVariant(const QDateTime &dateTime, DataStore *dataStore)
{
    switch (DbType::type(dataStore->database())) {
    case DbType::MySQL:
        // The QMYSQL driver does not encode timezone information. The MySQL server, when given time
        // without a timezones treats it as a local time. Thus, when we pass UTC time to QtSQL, it
        // will get interpreted as local time by MySQL, converted to UTC (again) and stored.
        // This causes two main problems:
        //  1) The query fails if the time sent by the driver is not valid in the local timezone (DST change), and
        //  2) It requires additional code when reading results from the database to re-interpret the returned
        //     "local time" as being UTC
        // So, instead we make sure we always pass real local time to MySQL, exactly as it expects it.
        return dateTime.toLocalTime();
    case DbType::Sqlite:
        // The QSQLITE converts QDateTime to string using QDateTime::ISODateWithMs and SQLite stores it as TEXT
        // (since it doesn't really have any dedicated date/time data type).
        // To stay consistent with other drivers, we convertr the dateTime to UTC here so SQLite stores UTC.
        return dateTime.toUTC();
    case DbType::PostgreSQL:
        // The QPSQL driver automatically converts the date time to UTC and forcibly stores it with UTC timezone
        // information, so we are good here. At least someone gets it right...
        return QVariant(dateTime);
    default:
        Q_UNREACHABLE();
        return {};
    }
}

/**
 * Returns the socket @p directory that is passed to this method or the one
 * the user has overwritten via the config file.
 * The passed @p fnLengthHint will also ensure the absolute file path length of the
 * directory + separator + hint would not overflow the system limitation.
 */
QString preferredSocketDirectory(const QString &directory, int fnLengthHint = -1);

/**
 * Returns name of filesystem that @p directory is stored on. This
 * only works on Linux and returns empty string on other platforms or when it's
 * unable to detect the filesystem.
 */
QString getDirectoryFileSystem(const QString &directory);

/**
 * Disables filesystem copy-on-write feature on given file or directory.
 * Only works on Linux and does nothing on other platforms.
 *
 * It was tested only with Btrfs but in theory can be called on any FS that
 * supports NOCOW.
 */
void disableCoW(const QString &path);

} // namespace Utils
} // namespace Server
} // namespace Akonadi
