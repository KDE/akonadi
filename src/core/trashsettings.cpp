/*
 Copyright (c) 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

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

#include "trashsettings.h"

#include <KConfig>
#include <KConfigGroup>

#include <QHash>
#include <QString>

using namespace Akonadi;

Akonadi::Collection TrashSettings::getTrashCollection(const QString &resource)
{
    KConfig config(QStringLiteral("akonaditrashrc"));
    KConfigGroup group(&config, resource);
    const Akonadi::Entity::Id colId = group.readEntry<Akonadi::Entity::Id> ("TrashCollection", -1);
    qWarning() << resource << colId;
    return Collection(colId);
}

void TrashSettings::setTrashCollection(const QString &resource, const Akonadi::Collection &collection)
{
    KConfig config(QStringLiteral("akonaditrashrc"));
    KConfigGroup group(&config, resource);
    qWarning() << resource << collection.id();
    group.writeEntry("TrashCollection", collection.id());
}
