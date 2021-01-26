/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectiongeneralpropertiespage_p.h"

#include "collection.h"
#include "collectionstatistics.h"
#include "collectionutils.h"
#include "entitydisplayattribute.h"

#include <KLocalizedString>

#include <KFormat>

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

    if (iconName.isEmpty()) {
        ui.customIcon->setIcon(CollectionUtils::defaultIconName(collection));
    } else {
        ui.customIcon->setIcon(iconName);
    }
    ui.customIconCheckbox->setChecked(!iconName.isEmpty());

    if (collection.statistics().count() >= 0) {
        ui.countLabel->setText(i18ncp("@label", "One object", "%1 objects", collection.statistics().count()));
        ui.sizeLabel->setText(KFormat().formatByteSize(collection.statistics().size()));
    } else {
        ui.statsBox->hide();
    }
}

void CollectionGeneralPropertiesPage::save(Collection &collection)
{
    if (collection.hasAttribute<EntityDisplayAttribute>() && !collection.attribute<EntityDisplayAttribute>()->displayName().isEmpty()) {
        collection.attribute<EntityDisplayAttribute>()->setDisplayName(ui.nameEdit->text());
    } else {
        collection.setName(ui.nameEdit->text());
    }

    if (ui.customIconCheckbox->isChecked()) {
        collection.attribute<EntityDisplayAttribute>(Collection::AddIfMissing)->setIconName(ui.customIcon->icon());
    } else if (collection.hasAttribute<EntityDisplayAttribute>()) {
        collection.attribute<EntityDisplayAttribute>()->setIconName(QString());
    }
}

//@endcond

#include "moc_collectiongeneralpropertiespage_p.cpp"
