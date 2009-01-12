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

#ifndef AKONADI_COLLECTIONDIALOG_H
#define AKONADI_COLLECTIONDIALOG_H

#include "akonadi_export.h"

#include <kdialog.h>

#include <akonadi/collection.h>

#include <QtGui/QAbstractItemView>

namespace Akonadi {

/**
 * @short A collection selection dialog.
 *
 * Provides a user (and developer) friendly way to select collections.
 *
 * @author Ingo Klöcker <kloecker@kde.org>
 * @since 4.3
 */
class AKONADI_EXPORT CollectionDialog : public KDialog
{
    Q_OBJECT
    Q_DISABLE_COPY( CollectionDialog )

  public:

    /**
     * Constructs a CollectionRequester widget.
     */
    explicit CollectionDialog( QWidget *parent = 0 );

    /**
     * Destructs the CollectionRequester.
     */
    ~CollectionDialog();

    /**
     * @returns The selected collection.
     */
    Akonadi::Collection selectedCollection() const;

    /**
     * @returns The list of selected collections.
     */
    Akonadi::Collection::List selectedCollections() const;

    /**
     * Sets the mime types any of which the selected collection(s) shall support.
     * @see Akonadi::CollectionDialog::setMimeTypeFilter()
     */
    void setMimeTypeFilter( const QStringList &mimeTypes );

    /**
     * @returns the mime types any of which the selected collection(s) shall support.
     * @see Akonadi::CollectionDialog::mimeTypeFilter()
     */
    QStringList mimeTypeFilter() const;

    /**
     * Sets the selection mode.
     * @see QAbstractItemView::setSelectionMode()
     */
    void setSelectionMode( QAbstractItemView::SelectionMode mode );

    /**
     * @returns The selection mode.
     * @see QAbstractItemView::selectionMode()
     */
    QAbstractItemView::SelectionMode selectionMode() const;


  private:
    class Private;
    Private * const d;

};

} // namespace Akonadi

#endif // AKONADI_COLLECTIONDIALOG_H
