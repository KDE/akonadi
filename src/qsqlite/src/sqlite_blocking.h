/*
    SPDX-FileCopyrightText: 2009 Bertjan Broeksema <broeksema@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SQLITE_BLOCKING_H
#define SQLITE_BLOCKING_H

#include <QtCore/QString>
#include <QtCore/QThread>

QString debugString();

struct sqlite3;
struct sqlite3_stmt;

int sqlite3_blocking_prepare16_v2(sqlite3 *db,            /* Database handle. */
                                  const void *zSql,      /* SQL statement, UTF-16 encoded */
                                  int nSql,              /* Length of zSql in bytes. */
                                  sqlite3_stmt **ppStmt, /* OUT: A pointer to the prepared statement */
                                  const void **pzTail    /* OUT: Pointer to unused portion of zSql */);

int sqlite3_blocking_step(sqlite3_stmt *pStmt);

#endif // SQLITE_BLOCKING_H
