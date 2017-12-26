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

#include "typehelper.h"
#include "nodetree.h"

#include <QString>
#include <QMetaType>

bool TypeHelper::isNumericType(const QString &name)
{
    const int metaTypeId = QMetaType::type(qPrintable(name));
    if (metaTypeId == -1) {
        return false;
    }

    switch (metaTypeId) {
    case QMetaType::Int:
    case QMetaType::UInt:
    case QMetaType::Double:
    case QMetaType::Long:
    case QMetaType::LongLong:
    case QMetaType::Short:
    case QMetaType::ULong:
    case QMetaType::ULongLong:
    case QMetaType::UShort:
    case QMetaType::Float:
        return true;
    default:
        return false;
    }
}

bool TypeHelper::isBoolType(const QString &name)
{
    const int metaTypeId = QMetaType::type(qPrintable(name));
    if (metaTypeId == -1) {
        return false;
    }

    switch (metaTypeId) {
    case QMetaType::Bool:
        return true;
    default:
        return false;
    }
}

bool TypeHelper::isBuiltInType(const QString &type)
{
    // TODO: should be smarter than this....
    return !type.startsWith(QLatin1String("Akonadi::Protocol"))
            || type == QLatin1String("Akonadi::Protocol::Attributes") // typedef to QMap
            || (type.startsWith(QLatin1String("Akonadi::Protocol")) // enums
                    && type.count(QStringLiteral("::")) > 2);
}

bool TypeHelper::isContainer(const QString &type)
{
    const int tplB = type.indexOf(QLatin1Char('<'));
    const int tplE = type.lastIndexOf(QLatin1Char('>'));
    return tplB > -1 && tplE > -1 && tplB < tplE;
}

bool TypeHelper::isAssociativeContainer(const QString &type)
{
    const int tplB = type.indexOf(QLatin1Char('<'));
    const int tplE = type.lastIndexOf(QLatin1Char('>'));
    return tplB > -1 && tplE > -1 && tplB < tplE && type.midRef(tplB, tplE).contains(QLatin1Char(','));
}

QString TypeHelper::containerType(const QString &type)
{
    const int tplB = type.indexOf(QLatin1Char('<'));
    const int tplE = type.indexOf(QLatin1Char('>'));
    return type.mid(tplB + 1, tplE - tplB - 1);
}

QString TypeHelper::containerName(const QString &type)
{
    const int tplB = type.indexOf(QLatin1Char('<'));
    return type.left(tplB);
}

bool TypeHelper::isPointerType(const QString &type)
{
    return type.endsWith(QLatin1String("Ptr"));
}
