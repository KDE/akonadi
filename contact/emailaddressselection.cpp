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

#include "emailaddressselection.h"
#include "emailaddressselection_p.h"

#include <kabc/contactgroup.h>
#include <kmime/kmime_header_parsing.h>

using namespace Akonadi;

EmailAddressSelection::Private::Private()
  : QSharedData()
{
}

EmailAddressSelection::Private::Private( const Private &other )
  : QSharedData( other )
{
  mName = other.mName;
  mEmailAddress = other.mEmailAddress;
  mItem = other.mItem;
}

EmailAddressSelection::EmailAddressSelection()
  : d( new Private )
{
}

EmailAddressSelection::EmailAddressSelection( const EmailAddressSelection &other )
  : d( other.d )
{
}

EmailAddressSelection& EmailAddressSelection::operator=( const EmailAddressSelection &other )
{
  if ( this != &other ) {
    d = other.d;
  }

  return *this;
}

EmailAddressSelection::~EmailAddressSelection()
{
}

bool EmailAddressSelection::isValid() const
{
  return d->mItem.isValid();
}

QString EmailAddressSelection::name() const
{
  return d->mName;
}

QString EmailAddressSelection::email() const
{
  return d->mEmailAddress;
}

QString EmailAddressSelection::quotedEmail() const
{
  if ( d->mItem.hasPayload<KABC::ContactGroup>() ) {
    if ( d->mEmailAddress == d->mName ) {
      return d->mName;
    }
  }

  KMime::Types::Mailbox mailbox;
  mailbox.setAddress( d->mEmailAddress.toUtf8() );
  mailbox.setName( d->mName );

  return mailbox.prettyAddress( KMime::Types::Mailbox::QuoteWhenNecessary );
}

Akonadi::Item EmailAddressSelection::item() const
{
  return d->mItem;
}
