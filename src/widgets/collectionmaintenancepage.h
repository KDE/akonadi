/*
   SPDX-FileCopyrightText: 2009-2021 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONMAINTENANCEPAGE_H
#define AKONADI_COLLECTIONMAINTENANCEPAGE_H

#include "akonadiwidgets_export.h"
#include "collectionpropertiespage.h"

namespace Akonadi
{
class AKONADIWIDGETS_EXPORT CollectionMaintenancePage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    explicit CollectionMaintenancePage(QWidget *parent = nullptr);
    ~CollectionMaintenancePage() override;

    void load(const Akonadi::Collection &col) override;
    void save(Akonadi::Collection &col) override;

protected:
    void init(const Akonadi::Collection &);

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionMaintenancePageFactory, CollectionMaintenancePage)

}

#endif /* COLLECTIONMAINTENANCEPAGE_H */
