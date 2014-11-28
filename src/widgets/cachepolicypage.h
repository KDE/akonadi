/*
    Copyright (c) 2010 Till Adam <adam@kde.org>

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

#ifndef AKONADI_CACHEPOLICYPAGE_H
#define AKONADI_CACHEPOLICYPAGE_H

#include "akonadiwidgets_export.h"
#include "collectionpropertiespage.h"

namespace Akonadi {

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
        UserMode,     ///< A simplified UI for end-users will be provided.
        AdvancedMode  ///< An advanced UI for debugging will be provided.
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
    ~CachePolicyPage();

    /**
     * Checks if the cache policy page can actually handle the given @p collection.
     */
    bool canHandle(const Collection &collection) const Q_DECL_OVERRIDE;

    /**
     * Loads the page content from the given @p collection.
     */
    void load(const Collection &collection) Q_DECL_OVERRIDE;

    /**
     * Saves page content to the given @p collection.
     */
    void save(Collection &collection) Q_DECL_OVERRIDE;

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void slotIntervalValueChanged(int))
    Q_PRIVATE_SLOT(d, void slotCacheValueChanged(int))
    Q_PRIVATE_SLOT(d, void slotRetrievalOptionsGroupBoxDisabled(bool))

    //@endcond
};

AKONADI_COLLECTION_PROPERTIES_PAGE_FACTORY(CachePolicyPageFactory, CachePolicyPage)

}

#endif
