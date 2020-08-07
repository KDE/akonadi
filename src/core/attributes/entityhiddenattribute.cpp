/******************************************************************************
 *
 *  SPDX-FileCopyrightText: 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 *
 *****************************************************************************/

#include "entityhiddenattribute.h"

#include <QByteArray>

using namespace Akonadi;

class Q_DECL_HIDDEN EntityHiddenAttribute::Private {};

EntityHiddenAttribute::EntityHiddenAttribute() = default;

EntityHiddenAttribute::~EntityHiddenAttribute() = default;

QByteArray Akonadi::EntityHiddenAttribute::type() const
{
    return QByteArrayLiteral("HIDDEN");
}

EntityHiddenAttribute *EntityHiddenAttribute::clone() const
{
    return new EntityHiddenAttribute();
}

QByteArray EntityHiddenAttribute::serialized() const
{
    return QByteArray();
}

void EntityHiddenAttribute::deserialize(const QByteArray &data)
{
    Q_ASSERT(data.isEmpty());
    Q_UNUSED(data);
}
