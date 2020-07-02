/*
    SPDX-FileCopyrightText: 2009 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONCOMBOBOX_H
#define AKONADI_COLLECTIONCOMBOBOX_H

#include "akonadiwidgets_export.h"
#include "collection.h"

#include <QComboBox>

class QAbstractItemModel;

namespace Akonadi
{

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
 * contentMimeTypes << KContacts::Addressee::mimeType();
 * contentMimeTypes << KContacts::ContactGroup::mimeType();
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
class AKONADIWIDGETS_EXPORT CollectionComboBox : public QComboBox
{
    Q_OBJECT

public:
    /**
     * Creates a new collection combobox.
     *
     * @param parent The parent widget.
     */
    explicit CollectionComboBox(QWidget *parent = nullptr);

    /**
     * Creates a new collection combobox with a custom @p model.
     *
     * The filtering by content mime type and access rights is done
     * on top of the custom model.
     *
     * @param model The custom model to use.
     * @param parent The parent widget.
     */
    explicit CollectionComboBox(QAbstractItemModel *model, QWidget *parent = nullptr);

    /**
     * Destroys the collection combobox.
     */
    ~CollectionComboBox();

    /**
     * Sets the content @p mimetypes the collections shall be filtered by.
     */
    void setMimeTypeFilter(const QStringList &mimetypes);

    /**
     * Returns the content mimetype the collections are filtered by.
     * Don't assume this list has the original order.
     */
    Q_REQUIRED_RESULT QStringList mimeTypeFilter() const;

    /**
     * Sets the access @p rights the collections shall be filtered by.
     */
    void setAccessRightsFilter(Collection::Rights rights);

    /**
     * Returns the access rights the collections are filtered by.
     */
    Q_REQUIRED_RESULT Collection::Rights accessRightsFilter() const;

    /**
     * Sets the @p collection that shall be selected by default.
     */
    void setDefaultCollection(const Collection &collection);

    /**
     * Returns the current selection.
     */
    Q_REQUIRED_RESULT Akonadi::Collection currentCollection() const;

    /**
     * @since 4.12
     */
    void setExcludeVirtualCollections(bool b);
    /**
     * @since 4.12
     */
    Q_REQUIRED_RESULT bool excludeVirtualCollections() const;

Q_SIGNALS:
    /**
     * This signal is emitted whenever the current selection
     * has been changed.
     *
     * @param collection The current selection.
     */
    void currentChanged(const Akonadi::Collection &collection);

private:
    //@cond PRIVATE
    class Private;
    Private *const d;

    Q_PRIVATE_SLOT(d, void activated(int))
    Q_PRIVATE_SLOT(d, void activated(const QModelIndex &))
    //@endcond
};

}

#endif
