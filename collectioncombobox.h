/*
    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_COLLECTIONCOMBOBOX_H
#define AKONADI_COLLECTIONCOMBOBOX_H

#include "akonadi_export.h"

#include <akonadi/collection.h>
#include <kcombobox.h>

class QAbstractItemModel;

namespace Akonadi {

/**
 * @short A combobox for selecting an Akonadi collection.
 *
 * This widget provides a combobox to select a collection
 * from the Akonadi storage.
 * The available collections can be filtered by mime type and
 * access rights.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * QStringList contentMimeTypes;
 * contentMimeTypes << KABC::Addressee::mimeType();
 * contentMimeTypes << KABC::ContactGroup::mimeType();
 *
 * CollectionComboBox *box = new CollectionComboBox( this );
 * box->setMimeTypeFilter( contentMimeTypes );
 * box->setAccessRightsFilter( Collection::CanCreateItem );
 * ...
 *
 * const Collection collection = box->currentCollection();
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_EXPORT CollectionComboBox : public KComboBox
{
  Q_OBJECT

  public:
    /**
     * Creates a new collection combobox.
     *
     * @param parent The parent widget.
     */
    explicit CollectionComboBox( QWidget *parent = 0 );

    /**
     * Creates a new collection combobox with a custom @p model.
     *
     * The filtering by content mime type and access rights is done
     * on top of the custom model.
     *
     * @param model The custom model to use.
     * @param parent The parent widget.
     */
    explicit CollectionComboBox( QAbstractItemModel *model, QWidget *parent = 0 );

    /**
     * Destroys the collection combobox.
     */
    ~CollectionComboBox();

    /**
     * Sets the content @p mimetypes the collections shall be filtered by.
     */
    void setMimeTypeFilter( const QStringList &mimetypes );

    /**
     * Returns the content mimetype the collections are filtered by.
     */
    QStringList mimeTypeFilter() const;

    /**
     * Sets the access @p rights the collections shall be filtered by.
     */
    void setAccessRightsFilter( Collection::Rights rights );

    /**
     * Returns the access rights the collections are filtered by.
     */
    Collection::Rights accessRightsFilter() const;

    /**
     * Sets the @p collection that shall be selected by default.
     */
    void setDefaultCollection( const Collection &collection );

    /**
     * Returns the current selection.
     */
    Akonadi::Collection currentCollection() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the current selection
     * has been changed.
     *
     * @param collection The current selection.
     */
    void currentChanged( const Akonadi::Collection &collection );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void activated( int ) )
    Q_PRIVATE_SLOT( d, void activated( const QModelIndex& ) )
    //@endcond
};

}

#endif
