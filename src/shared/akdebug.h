/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKDEBUG_H
#define AKDEBUG_H

#include <QDebug>

/**
 * Writes an error message to stdout/err and to the server error log and
 * aborts the program immediately.
 */
QDebug akFatal();

/**
 * Writes an error messasge to stdout/err and to the server error log.
 */
QDebug akError();

/**
 * Writes a debug message to stdout/err.
 */
#ifndef QT_NO_DEBUG_OUTPUT
QDebug akDebug();
#else
inline QNoDebug akDebug() {
    return QNoDebug();
}
#endif

/**
 * Init and rotate error logs.
 */
void akInit(const QString &appName);

/**
 * Returns the contents of @p name environment variable if it is defined,
 * or @p defaultValue otherwise.
 */
QString getEnv(const char *name, const QString &defaultValue = QString());

#endif
