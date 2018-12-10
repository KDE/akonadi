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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef AKONADI_SERIALIZER_%{APPNAMEUC}_H
#define AKONADI_SERIALIZER_%{APPNAMEUC}_H

#include <QObject>

#include <AkonadiCore/ItemSerializerPlugin>

namespace Akonadi
{

class SerializerPlugin%{APPNAME} : public QObject
                                 , public ItemSerializerPlugin
{
    Q_OBJECT
    Q_INTERFACES(Akonadi::ItemSerializerPlugin)
    Q_PLUGIN_METADATA(IID "org.kde.akonadi.SerializerPlugin%{APPNAME}")

public:
    bool deserialize(Item &item, const QByteArray &label, QIODevice &data, int version) override;
    void serialize(const Item &item, const QByteArray &label, QIODevice &data, int &version) override;

    QSet<QByteArray> parts(const Item &item) const;
};

}

#endif
