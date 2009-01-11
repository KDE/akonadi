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

#include "akonadi_export.h"

#include <akonadi/collection.h>

#include <khbox.h>

namespace Akonadi {

/**
 * @short A widget to request an Akonadi collection from the user.
 *
 * This class is a widget showing a read-only lineedit displaying
 * the currently chosen collection and a button invoking a dialog
 * for choosing a collection.
 *
 * @author Ingo Klöcker <kloecker@kde.org>
 * @since 4.3
 */
class AKONADI_EXPORT CollectionRequester : public KHBox
{
    Q_OBJECT
    Q_DISABLE_COPY(CollectionRequester)

public:
    /**
     * Constructs a CollectionRequester widget.
     */
    explicit CollectionRequester( QWidget *parent = 0 );

    /**
     * Constructs a CollectionRequester widget with initial collection @p col.
     */
    explicit CollectionRequester( const Akonadi::Collection &col, QWidget *parent = 0 );

    /**
     * Destructs the CollectionRequester.
     */
    ~CollectionRequester();

    /**
     * @returns the currently chosen collection. The collection may be invalid.
     */
    Akonadi::Collection collection() const;

    /**
     * Sets the mime types any of which the selected collection shall support.
     * @see Akonadi::CollectionDialog::setMimeTypeFilter()
     */
    void setMimeTypeFilter( const QStringList &mimeTypes );

    /**
    * @returns the mime types any of which the selected collection shall support.
    * @see Akonadi::CollectionDialog::mimeTypeFilter()
    */
    QStringList mimeTypeFilter() const;


public Q_SLOTS:
    /**
     * Sets the collection in the requester to @p col.
     */
    void setCollection( const Akonadi::Collection& col );


private:
    class Private;
    Private * const d;

    Q_PRIVATE_SLOT(d, void _k_slotOpenDialog())

};

} // namespace Akonadi

#endif // AKONADI_COLLECTIONREQUESTER_H
