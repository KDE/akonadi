/*
    This file is part of Akonadi Contact.

    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_EMAILADDRESSSELECTIONVIEW_H
#define AKONADI_EMAILADDRESSSELECTIONVIEW_H

#include "akonadi-contact_export.h"

#include <akonadi/item.h>

#include <QtGui/QAbstractItemView>
#include <QtGui/QWidget>

class KLineEdit;
class QAbstractItemModel;
class QTreeView;

namespace Akonadi {

/**
 * @short A widget to select email addresses from Akonadi.
 *
 * This view allows the user to select an name and email address from
 * the Akonadi storage.
 * The selected addresses are returned as EmailAddressSelectionView::Selection objects
 * which encapsulate the name, email address and the Akonadi item that has been selected.
 *
 * Example:
 *
 * @code
 *
 * Akonadi::EmailAddressSelectionView *view = new Akonadi::EmailAddressSelectionView( this );
 * view->view()->setSelectionMode( QAbstractItemView::MultiSelection );
 * ...
 *
 * const Akonadi::EmailAddressSelectionView::Selection::List selections = view->selectedAddresses();
 * foreach ( const Akonadi::EmailAddressSelectionView::Selection &selection, selections ) {
 *   qDebug() << "Name:" << selection.name() << "Email:" << selection.email();
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
 //AK_REVIEW: rename to EmailAddressSelectionWidget
class AKONADI_CONTACT_EXPORT EmailAddressSelectionView : public QWidget
{
  Q_OBJECT

  public:
    /**
     * The selection of an email address.
     */
    //AK_REVIEW: move to EmailAddressSelection class
    class AKONADI_CONTACT_EXPORT Selection
    {
      public:
        /**
         * A list of selection objects.
         */
        typedef QList<Selection> List;

        /**
         * Creates a new selection.
         */
        Selection();

        /**
         * Creates a new selection from an @p other selection.
         */
        Selection( const Selection &other );

        /**
         * Replaces this selection with the @p other selection.
         */
        Selection &operator=( const Selection &other );

        /**
         * Destroys the selection.
         */
        ~Selection();

        /**
         * Returns whether the selection is valid.
         */
        bool isValid() const;

        /**
         * Returns the name that is associated with the selected email address.
         */
        QString name() const;

        /**
         * Returns the address part of the selected email address.
         *
         * @note If a contact group has been selected, the name of the contact
         *       group is returned here and must be expanded by the caller.
         */
        QString email() const;

        /**
         * Returns the name and email address together, properly quoted if needed.
         *
         * @note If a contact group has been selected, the name of the contact
         *       group is returned here and must be expanded by the caller.
         */
        QString quotedEmail() const;

        /**
         * Returns the Akonadi item that is associated with the selected email address.
         */
        Akonadi::Item item() const;

      private:
        //@cond PRIVATE
        friend class EmailAddressSelectionView;

        class Private;
        QSharedDataPointer<Private> d;
        //@endcond
    };

    /**
     * Creates a new email address selection view.
     *
     * @param parent The parent widget.
     */
    explicit EmailAddressSelectionView( QWidget *parent = 0 );

    /**
     * Creates a new email address selection view.
     *
     * @param model A custom, ContactsTreeModel based model to use.
     * @param parent The parent widget.
     */
    explicit EmailAddressSelectionView( QAbstractItemModel *model, QWidget *parent = 0 );

    /**
     * Destroys the email address selection view.
     */
    ~EmailAddressSelectionView();

    /**
     * Returns the list of selected email addresses.
     */
    Selection::List selectedAddresses() const;

    /**
     * Returns the line edit that is used for the search line.
     */
    KLineEdit *searchLineEdit() const;

    /**
     * Returns the tree view that is used to list the items.
     */
    QTreeView *view() const;

  private:
    //@cond PRIVATE
    class Private;
    Private * const d;
    //@endcond
};

}

#endif
