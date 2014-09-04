/*
    Copyright 2008 Ingo Klöcker <kloecker@kde.org>

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

#ifndef AKONADI_COLLECTIONREQUESTER_H
#define AKONADI_COLLECTIONREQUESTER_H

#include "akonadiwidgets_export.h"
#include "collection.h"
#include "collectiondialog.h"
#include <QWidget>

namespace Akonadi {

/**
 * @short A widget to request an Akonadi collection from the user.
 *
 * This class is a widget showing a read-only lineedit displaying
 * the currently chosen collection and a button invoking a dialog
 * for choosing a collection.
 *
 * Example:
 *
 * @code
 *
 * // create a collection requester to select a collection of contacts
 * Akonadi::CollectionRequester requester( Akonadi::Collection::root(), this );
 * requester.setMimeTypeFilter( QStringList() << QString( "text/directory" ) );
 *
 * ...
 *
 * const Akonadi::Collection collection = requester.collection();
 * if ( collection.isValid() ) {
 *   ...
 * }
 *
 * @endcode
 *
 * @author Ingo Klöcker <kloecker@kde.org>
 * @since 4.3
 */
class AKONADIWIDGETS_EXPORT CollectionRequester : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(CollectionRequester)

public:
    /**
     * Creates a collection requester.
     *
     * @param parent The parent widget.
     */
    explicit CollectionRequester(QWidget *parent = 0);

    /**
     * Creates a collection requester with an initial @p collection.
     *
     * @param collection The initial collection.
     * @param parent The parent widget.
     */
    explicit CollectionRequester(const Akonadi::Collection &collection, QWidget *parent = 0);

    /**
     * Destroys the collection requester.
     */
    ~CollectionRequester();

    /**
     * Returns the currently chosen collection, or an empty collection if none
     * none was chosen.
     */
    Akonadi::Collection collection() const;

    /**
     * Sets the mime types any of which the selected collection shall support.
     */
    void setMimeTypeFilter(const QStringList &mimeTypes);

    /**
     * Returns the mime types any of which the selected collection shall support.
     */
    QStringList mimeTypeFilter() const;

    /**
     * Sets the access @p rights that the listed collections shall match with.
     * @param rights the access rights to set
     * @since 4.4
     */
    void setAccessRightsFilter(Collection::Rights rights);

    /**
     * Returns the access rights that the listed collections shall match with.
     * @since 4.4
     */
    Collection::Rights accessRightsFilter() const;

    /**
     * @param options new collection dialog options
     */
    void changeCollectionDialogOptions(CollectionDialog::CollectionDialogOptions options);

protected:
    void changeEvent(QEvent *event);

public Q_SLOTS:
    /**
     * Sets the @p collection of the requester.
     */
    void setCollection(const Akonadi::Collection &collection);

Q_SIGNALS:
    /**
     * This signal is emitted when the selected collection has changed.
     *
     * @param collection The selected collection.
     *
     * @since 4.5
     */
    void collectionChanged(const Akonadi::Collection &collection);

private:
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void _k_slotOpenDialog())
    Q_PRIVATE_SLOT(d, void _k_collectionReceived(KJob *job))
    Q_PRIVATE_SLOT(d, void _k_collectionsNamesReceived(KJob *job))
};

} // namespace Akonadi

#endif // AKONADI_COLLECTIONREQUESTER_H
