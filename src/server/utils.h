/*
 * SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 *
 */

#pragma once

#include <QDateTime>
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
    if (variant.type() == QVariant::String) {
        return variant.toString();
    } else if (variant.type() == QVariant::ByteArray) {
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
    if (variant.type() == QVariant::String) {
        return variant.toString().toUtf8();
    } else if (variant.type() == QVariant::ByteArray) {
        return variant.toByteArray();
    } else {
        qWarning("Unable to convert variant of type %s to QByteArray", variant.typeName());
        Q_ASSERT(false);
        return QByteArray();
    }
}

static inline QDateTime variantToDateTime(const QVariant &variant)
{
    if (variant.canConvert(QVariant::DateTime)) {
        // MySQL and SQLite backends read the datetime from the database and
        // assume it's local time. We stored it as UTC though, so we just need
        // to change the interpretation in QDateTime.
        // PostgreSQL on the other hand reads the datetime and assumes it's
        // UTC(?) and converts it to local time via QDateTime::toLocalTime(),
        // so we need to convert it back to UTC manually.
        switch (DbType::type(DataStore::self()->database())) {
        case DbType::MySQL:
        case DbType::Sqlite: {
            QDateTime dt = variant.toDateTime();
            dt.setTimeSpec(Qt::UTC);
            return dt;
        }
        case DbType::PostgreSQL:
            return variant.toDateTime().toUTC();
        default:
            Q_UNREACHABLE();
        }
    } else {
        qWarning("Unable to convert variant of type %s to QDateTime", variant.typeName());
        Q_ASSERT(false);
        return QDateTime();
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
