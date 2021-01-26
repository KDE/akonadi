/*
    SPDX-FileCopyrightText: 2010 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_CACHEPOLICYPAGE_H
#define AKONADI_CACHEPOLICYPAGE_H

#include "akonadiwidgets_export.h"
#include "collectionpropertiespage.h"

namespace Akonadi
{
/**
 * @short A page in a collection properties dialog to configure the cache policy.
 *
 * This page allows the user to fine tune the cache policy of a collection
 * in the Akonadi storage. It provides two modes, a UserMode and an AdvancedMode.
 * While the former should be used in end-user applications, the latter can be
 * used in debugging tools.
 *
 * @see Akonadi::CollectionPropertiesDialog, Akonadi::CollectionPropertiesPageFactory
 *
 * @author Till Adam <adam@kde.org>
 * @since 4.6
 */
class AKONADIWIDGETS_EXPORT CachePolicyPage : public CollectionPropertiesPage
{
    Q_OBJECT

public:
    /**
     * Describes the mode of the cache policy page.
     */
    enum GuiMode {
        UserMode, ///< A simplified UI for end-users will be provided.
        AdvancedMode ///< An advanced UI for debugging will be provided.
    };

    /**
     * Creates a new cache policy page.
     *
     * @param parent The parent widget.
     * @param mode The UI mode that will be used for the page.
     */
    explicit CachePolicyPage(QWidget *parent, GuiMode mode = UserMode);

    /**
     * Destroys the cache policy page.
     */
    ~CachePolicyPage() override;

    /**
     * Checks if the cache policy page can actually handle the given @p collection.
     */
    Q_REQUIRED_RESULT bool canHandle(const Collection &collection) const override;

    /**
     * Loads the page content from the given @p collection.
     */
    void load(const Collection &collection) override;

    /**
     * Saves page content to the given @p collection.
     */
    void save(Collection &collection) override;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;
    //@endcond
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CachePolicyPageFactory, CachePolicyPage)

}

#endif
