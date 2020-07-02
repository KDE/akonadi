/*
    SPDX-FileCopyrightText: 2008 Ingo Klöcker <kloecker@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_COLLECTIONREQUESTER_H
#define AKONADI_COLLECTIONREQUESTER_H

#include "akonadiwidgets_export.h"
#include "collection.h"
#include "collectiondialog.h"
#include <QWidget>

namespace Akonadi
{

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
    explicit CollectionRequester(QWidget *parent = nullptr);

    /**
     * Creates a collection requester with an initial @p collection.
     *
     * @param collection The initial collection.
     * @param parent The parent widget.
     */
    explicit CollectionRequester(const Akonadi::Collection &collection, QWidget *parent = nullptr);

    /**
     * Destroys the collection requester.
     */
    ~CollectionRequester() override;

    /**
     * Returns the currently chosen collection, or an empty collection if none
     * none was chosen.
     */
    Q_REQUIRED_RESULT Akonadi::Collection collection() const;

    /**
     * Sets the mime types any of which the selected collection shall support.
     */
    void setMimeTypeFilter(const QStringList &mimeTypes);

    /**
     * Returns the mime types any of which the selected collection shall support.
     */
    Q_REQUIRED_RESULT QStringList mimeTypeFilter() const;

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
    Q_REQUIRED_RESULT Collection::Rights accessRightsFilter() const;

    /**
     * @param options new collection dialog options
     */
    void changeCollectionDialogOptions(CollectionDialog::CollectionDialogOptions options);

    /**
     * Allow to specify collection content mimetype when we create new one.
     * @since 4.14.6
     */
    void setContentMimeTypes(const QStringList &mimetypes);

protected:
    void changeEvent(QEvent *event) override;

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
};

} // namespace Akonadi

#endif // AKONADI_COLLECTIONREQUESTER_H
