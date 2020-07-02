/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKOANDI_COLLECTIONGENERALPROPERTIESPAGE_P_H
#define AKOANDI_COLLECTIONGENERALPROPERTIESPAGE_P_H

#include "collectionpropertiespage.h"
#include "ui_collectiongeneralpropertiespage.h"

namespace Akonadi
{

//@cond PRIVATE

/**
 * @internal
 */
class CollectionGeneralPropertiesPage : public CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionGeneralPropertiesPage(QWidget *parent = nullptr);

    void load(const Collection &collection) override;
    void save(Collection &collection) override;

private:
    Ui::CollectionGeneralPropertiesPage ui;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionGeneralPropertiesPageFactory, CollectionGeneralPropertiesPage)
//@endcond

}

#endif
