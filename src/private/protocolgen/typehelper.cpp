/*
    SPDX-FileCopyrightText: 2017 Daniel Vrátil <dvratil@kde.og>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "typehelper.h"
#include "nodetree.h"

#include <QMetaType>
#include <QString>

bool TypeHelper::isNumericType(const QString &name)
{
    const int metaTypeId = QMetaType::fromName(qPrintable(name)).id();
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
    const int metaTypeId = QMetaType::fromName(qPrintable(name)).id();
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
    return !type.startsWith(QLatin1StringView("Akonadi::Protocol")) || type == QLatin1StringView("Akonadi::Protocol::Attributes") // typedef to QMap
        || (type.startsWith(QLatin1StringView("Akonadi::Protocol")) // enums
            && type.count(QStringLiteral("::")) > 2);
}

bool TypeHelper::isContainer(const QString &type)
{
    const int tplB = type.indexOf(u'<');
    const int tplE = type.lastIndexOf(u'>');
    return tplB > -1 && tplE > -1 && tplB < tplE;
}

QString TypeHelper::containerType(const QString &type)
{
    const int tplB = type.indexOf(u'<');
    const int tplE = type.indexOf(u'>');
    return type.mid(tplB + 1, tplE - tplB - 1);
}

QString TypeHelper::containerName(const QString &type)
{
    const int tplB = type.indexOf(u'<');
    return type.left(tplB);
}

bool TypeHelper::isPointerType(const QString &type)
{
    return type.endsWith(QLatin1StringView("Ptr"));
}
