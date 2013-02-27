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

#include "emailaddressselectiondialog.h"

using namespace Akonadi;

class EmailAddressSelectionDialog::Private
{
  public:
    Private( EmailAddressSelectionDialog *qq, QAbstractItemModel *model )
      : q( qq )
    {
      if ( model ) {
        mView = new EmailAddressSelectionWidget( model, q );
      } else {
        mView = new EmailAddressSelectionWidget( q );
      }
      q->connect( mView, SIGNAL(doubleClicked()), q, SLOT(accept()));
      q->setButtons( Ok | Cancel );
      q->setMainWidget( mView );
    }

    EmailAddressSelectionDialog *q;
    EmailAddressSelectionWidget *mView;
};

EmailAddressSelectionDialog::EmailAddressSelectionDialog( QWidget *parent )
  : KDialog( parent ), d( new Private( this, 0 ) )
{
}

EmailAddressSelectionDialog::EmailAddressSelectionDialog( QAbstractItemModel *model, QWidget *parent )
  : KDialog( parent ), d( new Private( this, model ) )
{
}

EmailAddressSelectionDialog::~EmailAddressSelectionDialog()
{
  delete d;
}

EmailAddressSelection::List EmailAddressSelectionDialog::selectedAddresses() const
{
  return d->mView->selectedAddresses();
}

EmailAddressSelectionWidget* EmailAddressSelectionDialog::view() const
{
  return d->mView;
}
