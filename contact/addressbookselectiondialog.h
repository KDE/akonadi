/*
    This file is part of Akonadi Contact.

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

#ifndef AKONADI_ADDRESSBOOKSELECTIONDIALOG_H
#define AKONADI_ADDRESSBOOKSELECTIONDIALOG_H

#include "akonadi-contact_export.h"

#include <kdialog.h>

namespace Akonadi
{

class Collection;

//TODO_AKONADI_REVIEW: drop this class and extend collectiondialog
/**
 * @short A dialog to select an address book in Akonadi.
 *
 * This dialog provides a combobox to select an address book
 * collection from the Akonadi storage.
 * The available address books can be filtered to show only
 * address books that can contain contacts, contact groups or both.
 *
 * Example:
 *
 * @code
 *
 * using namespace Akonadi;
 *
 * AddressBookSelectionDialog dlg( AddressBookSelectionDialog::All, this );
 * if ( dlg.exec() ) {
 *   const Collection addressBook = dlg.selectedAddressBook();
 *   ...
 * }
 *
 * @endcode
 *
 * @author Tobias Koenig <tokoe@kde.org>
 * @since 4.4
 */
class AKONADI_CONTACT_EXPORT AddressBookSelectionDialog : public KDialog
{
  public:
    /**
     * Describes what type of address books shall be listed for
     * selection.
     */
    enum Type
    {
      ContactsOnly,      ///< Only address books that can store contacts.
      ContactGroupsOnly, ///< Only address books that can store contact groups.
      All                ///< All address books.
    };

    /**
     * Creates a new address bool selection dialog.
     *
     * @param parent The parent widget.
     */
    explicit AddressBookSelectionDialog( Type type, QWidget *parent = 0 );

    /**
     * Destroys the address book selection dialog.
     */
    ~AddressBookSelectionDialog();

    /**
     * Sets the @p addressbook that shall be selected as default.
     */
    void setDefaultAddressBook( const Akonadi::Collection &addressbook );

    /**
     * Returns the selected collection or an invalid collection
     * if none is selected.
     */
    Akonadi::Collection selectedAddressBook() const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif
