/******************************************************************************
 *
 *  Copyright (c) 2009 Szymon Stefanek <s.stefanek at gmail dot com>
 *
 *  This library is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  This library is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to the
 *  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA, 02110-1301, USA.
 *
 *****************************************************************************/

#include "entityhiddenattribute.h"

#include <QtCore/QByteArray>

using namespace Akonadi;

EntityHiddenAttribute::EntityHiddenAttribute()
    : d(0)
{
}

EntityHiddenAttribute::~EntityHiddenAttribute()
{
}

QByteArray Akonadi::EntityHiddenAttribute::type() const
{
    static const QByteArray sType( "HIDDEN" );
    return sType;
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
