/*
 * Copyright (C) %{CURRENT_YEAR} by %{AUTHOR} <%{EMAIL}>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "akonadi_serializer_%{APPNAMELC}.h"

#include <AkonadiCore/Item>

using namespace Akonadi;

bool SerializerPlugin%{APPNAME}::deserialize(Item &item, const QByteArray &label, QIODevice &data, int version)
{
    Q_UNUSED(item);
    Q_UNUSED(label);
    Q_UNUSED(data);
    Q_UNUSED(version);

    // TODO Implement this

    return false;
}

void SerializerPlugin%{APPNAME}::serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version)
{
    Q_UNUSED(item);
    Q_UNUSED(label);
    Q_UNUSED(data);
    Q_UNUSED(version);

    // TODO Implement this
}

QSet<QByteArray> SerializerPlugin%{APPNAME}::parts(const Item &item) const
{
    // only need to reimplement this when implementing partial serialization
    // i.e. when using the "label" parameter of the other two methods
    return ItemSerializerPlugin::parts(item);
}
