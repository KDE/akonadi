/*
   SPDX-FileCopyrightText: 2009-2022 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
#include "collectionpropertiespage.h"

#include <memory>

namespace Akonadi
{
class CollectionMaintenancePagePrivate;

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
    /// @cond PRIVATE
    std::unique_ptr<CollectionMaintenancePagePrivate> const d;
    /// @endcond
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionMaintenancePageFactory, CollectionMaintenancePage)

}

