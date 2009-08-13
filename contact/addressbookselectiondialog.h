/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef AKONADI_ADDRESSBOOKSELECTIONDIALOG_H
#define AKONADI_ADDRESSBOOKSELECTIONDIALOG_H

#include "akonadi-contact_export.h"

#include <kdialog.h>

namespace Akonadi
{

class Collection;

/**
 * @short A dialog to select an address book in Akonadi.
 *
 * @author Tobias Koenig <tokoe@kde.org>
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
