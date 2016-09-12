/*
    Copyright (c) 2017 Daniel Vrátil <dvratil@kde.og>

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

#ifndef TYPEHELPER_H
#define TYPEHELPER_H

class QString;
class PropertyNode;

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

}
#endif
