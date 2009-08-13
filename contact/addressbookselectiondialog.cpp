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

#include "addressbookselectiondialog.h"

#include "addressbookcombobox.h"

#include <akonadi/item.h>
#include <klocale.h>

#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>

using namespace Akonadi;

class AddressBookSelectionDialog::Private
{
  public:
    AddressBookComboBox *mCollectionCombo;
};

AddressBookSelectionDialog::AddressBookSelectionDialog( Type type, QWidget *parent )
  : KDialog( parent ), d( new Private )
{
  setCaption( i18n( "Select Address Book" ) );
  setButtons( Ok | Cancel );

  QWidget *mainWidget = new QWidget( this );
  setMainWidget( mainWidget );

  QVBoxLayout *layout = new QVBoxLayout( mainWidget );

  d->mCollectionCombo = new Akonadi::AddressBookComboBox( (AddressBookComboBox::Type)type );

  layout->addWidget( new QLabel( i18n( "Select the address book" ) ) );
  layout->addWidget( d->mCollectionCombo );
}

AddressBookSelectionDialog::~AddressBookSelectionDialog()
{
  delete d;
}

Akonadi::Collection AddressBookSelectionDialog::selectedAddressBook() const
{
  return d->mCollectionCombo->selectedAddressBook();
}
