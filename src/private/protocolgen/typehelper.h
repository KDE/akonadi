/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TYPEHELPER_H
#define TYPEHELPER_H

class QString;

namespace TypeHelper
{
bool isNumericType(const QString &name);
bool isBoolType(const QString &name);

/**
 * Returns true if @p node is of C++ or Qt type, C++ if it's a generated type
 */
bool isBuiltInType(const QString &type);

bool isContainer(const QString &type);

QString containerType(const QString &type);
QString containerName(const QString &type);
bool isPointerType(const QString &type);

}
#endif
