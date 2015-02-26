/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>
    Copyright 2010 Laurent Montel <montel@kde.org>

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

#ifndef AKONADI_COLLECTIONDIALOG_H
#define AKONADI_COLLECTIONDIALOG_H

#include "akonadi_export.h"

#include <kdialog.h>

#include <akonadi/collection.h>

#include <QAbstractItemView>

namespace Akonadi {

/**
 * @short A collection selection dialog.
 *
 * Provides a dialog that lists collections that are available
 * on the Akonadi storage and allows the selection of one or multiple
 * collections.
 *
 * The list of shown collections can be filtered by mime type and access
 * rights. Note that mime types are not enabled by default, so
 * setMimeTypeFilter() must be called to enable the desired mime types.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * // Show the user a dialog to select a writable collection of contacts
 * CollectionDialog dlg( this );
 * dlg.setMimeTypeFilter( QStringList() << KABC::Addressee::mimeType() );
 * dlg.setAccessRightsFilter( Collection::CanCreateItem );
 * dlg.setDescription( i18n( "Select an address book for saving:" ) );
 *
 * if ( dlg.exec() ) {
 *   const Collection collection = dlg.selectedCollection();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Ingo Klöcker <kloecker@kde.org>
 * @since 4.3
 */
class AKONADI_EXPORT CollectionDialog : public KDialog
{
    Q_OBJECT
    Q_DISABLE_COPY(CollectionDialog)

public:
    /* @since 4.6
     */
    enum CollectionDialogOption {
        None = 0,
        AllowToCreateNewChildCollection = 1,
        KeepTreeExpanded = 2
    };

    Q_DECLARE_FLAGS(CollectionDialogOptions, CollectionDialogOption)

    /**
     * Creates a new collection dialog.
     *
     * @param parent The parent widget.
     */
    explicit CollectionDialog(QWidget *parent = 0);

    /**
     * Creates a new collection dialog with a custom @p model.
     *
     * The filtering by content mime type and access rights is done
     * on top of the custom model.
     *
     * @param model The custom model to use.
     * @param parent The parent widget.
     *
     * @since 4.4
     */
    explicit CollectionDialog(QAbstractItemModel *model, QWidget *parent = 0);

    /**
     * Creates a new collection dialog with a custom @p model.
     *
     * The filtering by content mime type and access rights is done
     * on top of the custom model.
     *
     * @param options The collection dialog options.
     * @param model The custom model to use.
     * @param parent The parent widget.
     *
     * @since 4.6
     */

    explicit CollectionDialog(CollectionDialogOptions options, QAbstractItemModel *model = 0, QWidget *parent = 0);

    /**
     * Destroys the collection dialog.
     */
    ~CollectionDialog();

    /**
     * Sets the mime types any of which the selected collection(s) shall support.
     * Note that mime types are not enabled by default.
     * @param mimeTypes MIME type filter values
     */
    void setMimeTypeFilter(const QStringList &mimeTypes);

    /**
     * Returns the mime types any of which the selected collection(s) shall support.
     */
    QStringList mimeTypeFilter() const;

    /**
     * Sets the access @p rights that the listed collections shall match with.
     * @param rights access rights filter values
     * @since 4.4
     */
    void setAccessRightsFilter(Collection::Rights rights);

    /**
     * Sets the access @p rights that the listed collections shall match with.
     *
     * @since 4.4
     */
    Collection::Rights accessRightsFilter() const;

    /**
     * Sets the @p text that will be shown in the dialog.
     * @param text the dialog's description text
     * @since 4.4
     */
    void setDescription(const QString &text);

    /**
     * Sets the @p collection that shall be selected by default.
     * @param collection the dialog's pre-selected collection
     * @since 4.4
     */
    void setDefaultCollection(const Collection &collection);

    /**
     * Sets the selection mode. The initial default mode is
     * QAbstractItemView::SingleSelection.
     * @param mode the selection mode to use
     * @see QAbstractItemView::setSelectionMode()
     */
    void setSelectionMode(QAbstractItemView::SelectionMode mode);

    /**
     * Returns the selection mode.
     * @see QAbstractItemView::selectionMode()
     */
    QAbstractItemView::SelectionMode selectionMode() const;

    /**
     * Returns the selected collection if the selection mode is
     * QAbstractItemView::SingleSelection. If another selection mode was set,
     * or nothing is selected, an invalid collection is returned.
     */
    Akonadi::Collection selectedCollection() const;

    /**
     * Returns the list of selected collections.
     */
    Akonadi::Collection::List selectedCollections() const;

    /**
     * Change collection dialog options.
     * @param options the collection dialog options to change
     * @since 4.6
     */
    void changeCollectionDialogOptions(CollectionDialogOptions options);

    /**
      * @since 4.13
      */
    void setUseFolderByDefault(bool b);
    /**
      * @since 4.13
      */
    bool useFolderByDefault() const;
    /**
     * Allow to specify collection content mimetype when we create new one.
     * @since 4.14.6
     */
    void setContentMimeTypes(const QStringList &mimetypes);

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void slotCollectionAvailable(const QModelIndex &))
    Q_PRIVATE_SLOT(d, void slotSelectionChanged())
    Q_PRIVATE_SLOT(d, void slotAddChildCollection())
    Q_PRIVATE_SLOT(d, void slotCollectionCreationResult(KJob *job))
    Q_PRIVATE_SLOT(d, void slotFilterFixedString(const QString &))
    Q_PRIVATE_SLOT(d, void slotDoubleClicked())
    //@endcond
};

} // namespace Akonadi

#endif // AKONADI_COLLECTIONDIALOG_H
