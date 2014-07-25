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

#ifndef AKONADI_EMAILADDRESSSELECTIONWIDGET_H
#define AKONADI_EMAILADDRESSSELECTIONWIDGET_H

#include "akonadi-contact_export.h"

#include "emailaddressselection.h"

#include <akonadi/item.h>

#include <QAbstractItemView>
#include <QWidget>

class KLineEdit;
class QAbstractItemModel;
class QTreeView;

namespace Akonadi {

/**
 * @short A widget to select email addresses from Akonadi.
 *
 * This widget allows the user to select an name and email address from
 * the Akonadi storage.
 * The selected addresses are returned as EmailAddressSelectionWidget::Selection objects
 * which encapsulate the name, email address and the Akonadi item that has been selected.
 *
 * Example:
 *
 * @code
 *
 * Akonadi::EmailAddressSelectionWidget *widget = new Akonadi::EmailAddressSelectionWidget( this );
 * widget->view()->setSelectionMode( QAbstractItemView::MultiSelection );
 * ...
 *
 * foreach ( const Akonadi::EmailAddressSelection &selection, widget->selectedAddresses() ) {
 *   qDebug() << "Name:" << selection.name() << "Email:" << selection.email();
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.5
 */
class AKONADI_CONTACT_EXPORT EmailAddressSelectionWidget : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Creates a new email address selection widget.
     *
     * @param parent The parent widget.
     */
    explicit EmailAddressSelectionWidget( QWidget *parent = 0 );

    /**
     * Creates a new email address selection widget.
     *
     * @param model A custom, ContactsTreeModel based model to use.
     * @param parent The parent widget.
     */
    explicit EmailAddressSelectionWidget( QAbstractItemModel *model, QWidget *parent = 0 );

    /**
     * @brief Creates a new email address selection widget.
     * @param showOnlyContactWithEmail Allow to specify if you want to see only contact with email (by default yes in other constructor)
     * @param model A custom ContactsTreeModel based model to use.
     * @param parent The parent widget.
     */
    explicit EmailAddressSelectionWidget( bool showOnlyContactWithEmail, QAbstractItemModel *model = 0, QWidget *parent = 0 );

    /**
     * Destroys the email address selection widget.
     */
    ~EmailAddressSelectionWidget();

    /**
     * Returns the list of selected email addresses.
     */
    EmailAddressSelection::List selectedAddresses() const;

    /**
     * Returns the line edit that is used for the search line.
     */
    KLineEdit *searchLineEdit() const;

    /**
     * Returns the tree view that is used to list the items.
     */
    QTreeView *view() const;

  Q_SIGNALS:
    /**
     * @since 4.10.1
     */
    void doubleClicked();
  private:
    //@cond PRIVATE
    class Private;
    Private * const d;
    //@endcond
};

}

#endif
