/*
 SPDX-FileCopyrightText: 2011 Christian Mollekopf <chrigi_1@fastmail.fm>

 SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "trashsettings.h"
#include "akonadicore_debug.h"

#include <KConfig>
#include <KConfigGroup>

#include <QHash>
#include <QString>

using namespace Akonadi;

Akonadi::Collection TrashSettings::getTrashCollection(const QString &resource)
{
    KConfig config(QStringLiteral("akonaditrashrc"));
    KConfigGroup group(&config, resource);
    const Akonadi::Collection::Id colId = group.readEntry<Akonadi::Collection::Id> ("TrashCollection", -1);
    qCWarning(AKONADICORE_LOG) << resource << colId;
    return Collection(colId);
}

void TrashSettings::setTrashCollection(const QString &resource, const Akonadi::Collection &collection)
{
    KConfig config(QStringLiteral("akonaditrashrc"));
    KConfigGroup group(&config, resource);
    qCWarning(AKONADICORE_LOG) << resource << collection.id();
    group.writeEntry("TrashCollection", collection.id());
}
