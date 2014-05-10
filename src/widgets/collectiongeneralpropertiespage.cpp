/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "collectiongeneralpropertiespage_p.h"

#include "collection.h"
#include "entitydisplayattribute.h"
#include "collectionstatistics.h"
#include "collectionutils.h"

#include <klocale.h>
#include <klocalizedstring.h>
#include <kglobal.h>
#include <KLocale>

using namespace Akonadi;

//@cond PRIVATE

CollectionGeneralPropertiesPage::CollectionGeneralPropertiesPage(QWidget *parent)
    : CollectionPropertiesPage(parent)
{
    setObjectName(QStringLiteral("Akonadi::CollectionGeneralPropertiesPage"));

    setPageTitle(i18nc("@title:tab general properties page", "General"));
    ui.setupUi(this);
}

void CollectionGeneralPropertiesPage::load(const Collection &collection)
{
    QString displayName;
    QString iconName;
    if (collection.hasAttribute<EntityDisplayAttribute>()) {
        displayName = collection.attribute<EntityDisplayAttribute>()->displayName();
        iconName = collection.attribute<EntityDisplayAttribute>()->iconName();
    }

    if (displayName.isEmpty()) {
        ui.nameEdit->setText(collection.name());
    } else {
        ui.nameEdit->setText(displayName);
    }

#ifndef KDEPIM_MOBILE_UI
    if (iconName.isEmpty()) {
        ui.customIcon->setIcon(CollectionUtils::defaultIconName(collection));
    } else {
        ui.customIcon->setIcon(iconName);
    }
    ui.customIconCheckbox->setChecked(!iconName.isEmpty());
#endif

    if (collection.statistics().count() >= 0) {
        ui.countLabel->setText(i18ncp("@label", "One object", "%1 objects",
                                      collection.statistics().count()));
        ui.sizeLabel->setText(KLocale::global()->formatByteSize(collection.statistics().size()));
    } else {
        ui.statsBox->hide();
    }
}

void CollectionGeneralPropertiesPage::save(Collection &collection)
{
    if (collection.hasAttribute<EntityDisplayAttribute>() &&
        !collection.attribute<EntityDisplayAttribute>()->displayName().isEmpty()) {
        collection.attribute<EntityDisplayAttribute>()->setDisplayName(ui.nameEdit->text());
    } else {
        collection.setName(ui.nameEdit->text());
    }

#ifndef KDEPIM_MOBILE_UI
    if (ui.customIconCheckbox->isChecked()) {
        collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing)->setIconName(ui.customIcon->icon());
    } else if (collection.hasAttribute<EntityDisplayAttribute>()) {
        collection.attribute<EntityDisplayAttribute>()->setIconName(QString());
    }
#endif
}

//@endcond

#include "moc_collectiongeneralpropertiespage_p.cpp"
