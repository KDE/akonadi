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
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

#include <QtGui/QAbstractItemView>
#include <QtGui/QWidget>

class QAbstractItemModel;
class QTreeView;

namespace Akonadi {

/**
 * @short A widget to select email addresses from Akonadi.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_CONTACT_EXPORT EmailAddressSelectionView : public QWidget
{
  Q_OBJECT

  public:
    class Selection
    {
      public:
        typedef QList<Selection> List;

        Selection();
        Selection( const Selection &other );
        Selection &operator=( const Selection& );
        ~Selection();

        bool isValid() const;

        QString name() const;
        QString email() const;

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
     * Sets the selection @p mode of the address selection view.
     *
     * Possible values are QAbstractItemView::SingleSelection or QAbstractItemView::MultiSelection.
     */
    void setSelectionMode( QAbstractItemView::SelectionMode mode );

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
