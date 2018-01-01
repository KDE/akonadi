/*
   Copyright (C) 2009-2018 Montel Laurent <montel@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
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

