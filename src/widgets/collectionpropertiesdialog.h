/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
#include "collectionpropertiespage.h"

#include <QDialog>

namespace Akonadi
{
class Collection;

/**
 * @short A generic and extensible dialog for collection properties.
 *
 * This dialog allows you to show or modify the properties of a collection.
 *
 * @code
 *
 * Akonadi::Collection collection = ...
 *
 * CollectionPropertiesDialog dlg( collection, this );
 * dlg.exec();
 *
 * @endcode
 *
 * It can be extended by custom pages, which contains gui elements for custom
 * properties.
 *
 * @see Akonadi::CollectionPropertiesPage
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT CollectionPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * Enumerates the registered default pages which can be displayed.
     *
     * @since 4.7
     */
    enum DefaultPage {
        GeneralPage, //!< General properties page
        CachePage //!< Cache properties page
    };

    /**
     * Creates a new collection properties dialog.
     *
     * @param collection The collection which properties should be shown.
     * @param parent The parent widget.
     */
    explicit CollectionPropertiesDialog(const Collection &collection, QWidget *parent = nullptr);

    /**
     * Creates a new collection properties dialog.
     *
     * This constructor allows to specify the subset of registered pages that will
     * be shown as well as their order. The pages have to set an objectName in their
     * constructor to make it work. If an empty list is passed, all registered pages
     * will be loaded. Use defaultPageObjectName() to fetch the object name for a
     * registered default page.
     *
     * @param collection The collection which properties should be shown.
     * @param pages The object names of the pages that shall be loaded.
     * @param parent The parent widget.
     *
     * @since 4.6
     */
    CollectionPropertiesDialog(const Collection &collection, const QStringList &pages, QWidget *parent = nullptr);

    /**
     * Destroys the collection properties dialog.
     *
     * @note Never call manually, the dialog is deleted automatically once all changes
     *       are written back to the Akonadi storage.
     */
    ~CollectionPropertiesDialog();

    /**
     * Register custom pages for the collection properties dialog.
     *
     * @param factory The properties page factory that provides the custom page.
     *
     * @see Akonadi::CollectionPropertiesPageFactory
     */
    static void registerPage(CollectionPropertiesPageFactory *factory);

    /**
     * Sets whether to @p use default page or not.
     *
     * @since 4.4
     * @param use mode of default page's usage
     */
    static void useDefaultPage(bool use);

    /**
     * Returns the object name of one of the dialog's registered default pages.
     * The object name may be used in the QStringList constructor parameter to
     * specify which default pages should be shown.
     *
     * @param page the desired page
     * @return the page's object name
     *
     * @since 4.7
     */
    static QString defaultPageObjectName(DefaultPage page);

    /**
     * Sets the page to be shown in the tab widget.
     *
     * @param name The object name of the page that is to be shown.
     *
     * @since 4.10
     */
    void setCurrentPage(const QString &name);

Q_SIGNALS:
    void settingsSaved();

private:
    /// @cond PRIVATE
    class Private;
    Private *const d;
    /// @endcond
};

}

