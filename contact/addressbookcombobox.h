/*
    Copyright (c) 2007-2009 Tobias Koenig <tokoe@kde.org>

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

#ifndef AKONADI_ADDRESSBOOKCOMBOBOX_H
#define AKONADI_ADDRESSBOOKCOMBOBOX_H

#include "akonadi-contact_export.h"

#include <QtGui/QWidget>

#include <akonadi/collection.h>

class QAbstractItemModel;

namespace Akonadi {

/**
 * @short A combobox for selecting an Akonadi address book.
 *
 * @author Tobias Koenig <tokoe@kde.org>
 */
class AKONADI_CONTACT_EXPORT AddressBookComboBox : public QWidget
{
  Q_OBJECT

  public:
    /**
     * Creates a new address book combobox.
     *
     * @param parent The parent widget.
     */
    AddressBookComboBox( QWidget *parent = 0 );

    /**
     * Destroys the collection combobox.
     */
    ~AddressBookComboBox();

    /**
     * Sets the collection model.
     */
    void setModel( QAbstractItemModel *model );

    /**
     * Returns the selected address book.
     */
    Akonadi::Collection selectedAddressBook() const;

  Q_SIGNALS:
    /**
     * This signal is emitted whenever the selected address book changed.
     *
     * @param addressBook The selected address book.
     */
    void selectionChanged( const Akonadi::Collection &addressBook );

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;

    Q_PRIVATE_SLOT( d, void activated( int ) )
    //@endcond
};

}

#endif
