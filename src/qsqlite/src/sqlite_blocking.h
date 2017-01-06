/*
    Copyright (c) 2009 Bertjan Broeksema <broeksema@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
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
