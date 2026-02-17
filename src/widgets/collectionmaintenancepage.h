/*
   SPDX-FileCopyrightText: 2009-2026 Laurent Montel <montel@kde.org>

   SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
#include "collectionpropertiespage.h"

#include <memory>

namespace Akonadi
{
class CollectionMaintenancePagePrivate;

/*!
 * \brief The collection maintenance page for collection properties dialog.
 *
 * \class Akonadi::CollectionMaintenancePage
 * \inheaderfile Akonadi/CollectionMaintenancePage
 * \inmodule AkonadiWidgets
 */
class AKONADIWIDGETS_EXPORT CollectionMaintenancePage : public Akonadi::CollectionPropertiesPage
{
    Q_OBJECT
public:
    /*!
     * Creates a new collection maintenance page.\n
     * \a parent The parent widget.
     */
    explicit CollectionMaintenancePage(QWidget *parent = nullptr);
    /*!
     * Destroys the collection maintenance page.
     */
    ~CollectionMaintenancePage() override;

    /*!
     * Loads the page content from the given collection.
     * \a col The collection to load.
     */
    void load(const Akonadi::Collection &col) override;
    /*!
     * Saves the page content to the given collection.
     * \a col The collection to save to.
     */
    void save(Akonadi::Collection &col) override;

protected:
    /*!
     * Initializes the page with the given collection.
     * \a col The collection to initialize with.
     */
    void init(const Akonadi::Collection &);

private:
    std::unique_ptr<CollectionMaintenancePagePrivate> const d;
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CollectionMaintenancePageFactory, CollectionMaintenancePage)

}
