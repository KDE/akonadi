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

#include "contactsfilterproxymodel.h"

#include "contactstreemodel.h"

#include <akonadi/entitytreemodel.h>
#include <kabc/addressee.h>
#include <kabc/contactgroup.h>

static bool contactMatchesFilter( const KABC::Addressee &contact, const QString &filterString );
static bool contactGroupMatchesFilter( const KABC::ContactGroup &group, const QString &filterString );

using namespace Akonadi;

class ContactsFilterProxyModel::Private
{
  public:
    QString mFilter;
};

ContactsFilterProxyModel::ContactsFilterProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ), d( new Private )
{
  // contact names should be sorted correctly
  setSortLocaleAware( true );
}

ContactsFilterProxyModel::~ContactsFilterProxyModel()
{
  delete d;
}

void ContactsFilterProxyModel::setFilterString( const QString &filter )
{
  d->mFilter = filter;
  invalidateFilter();
}

bool ContactsFilterProxyModel::filterAcceptsRow( int row, const QModelIndex &parent ) const
{
  if ( d->mFilter.isEmpty() )
    return true;

  const QModelIndex index = sourceModel()->index( row, 0, parent );

  const Akonadi::Item item = index.data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();

  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();
    return contactMatchesFilter( contact, d->mFilter );
  } else if ( item.hasPayload<KABC::ContactGroup>() ) {
    const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
    return contactGroupMatchesFilter( group, d->mFilter );
  }

  return true;
}

bool ContactsFilterProxyModel::lessThan( const QModelIndex &leftIndex, const QModelIndex &rightIndex ) const
{
  const QDate leftDate = leftIndex.data( ContactsTreeModel::DateRole ).toDate();
  const QDate rightDate = rightIndex.data( ContactsTreeModel::DateRole ).toDate();

  if ( leftDate.isValid() && rightDate.isValid() ) {
    if ( leftDate.month() < rightDate.month() )
      return true;
    else if ( leftDate.month() == rightDate.month() )
      return (leftDate.day() < rightDate.day());
    else
      return false;
  }

  return QSortFilterProxyModel::lessThan( leftIndex, rightIndex );
}

static bool addressMatchesFilter( const KABC::Address &address, const QString &filterString )
{
  if ( address.street().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( address.locality().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( address.region().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( address.postalCode().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( address.country().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( address.label().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( address.postOfficeBox().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  return false;
}

static bool contactMatchesFilter( const KABC::Addressee &contact, const QString &filterString )
{
  if ( contact.assembledName().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.formattedName().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.nickName().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.birthday().toString().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  const KABC::Address::List addresses = contact.addresses();
  int count = addresses.count();
  for ( int i = 0; i < count; ++i ) {
    if ( addressMatchesFilter( addresses.at( i ), filterString ) )
      return true;
  }

  const KABC::PhoneNumber::List phoneNumbers = contact.phoneNumbers();
  count = phoneNumbers.count();
  for ( int i = 0; i < count; ++i ) {
    if ( phoneNumbers.at( i ).number().contains( filterString, Qt::CaseInsensitive ) )
      return true;
  }

  const QStringList emails = contact.emails();
  count = emails.count();
  for ( int i = 0; i < count; ++i ) {
    if ( emails.at( i ).contains( filterString, Qt::CaseInsensitive ) )
      return true;
  }

  if ( contact.mailer().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.title().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.role().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.organization().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.department().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.note().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  if ( contact.url().url().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  const QStringList customs = contact.customs();
  count = customs.count();
  for ( int i = 0; i < count; ++i ) {
    if ( customs.at( i ).contains( filterString, Qt::CaseInsensitive ) )
      return true;
  }

  return false;
}

bool contactGroupMatchesFilter( const KABC::ContactGroup &group, const QString &filterString )
{
  if ( group.name().contains( filterString, Qt::CaseInsensitive ) )
    return true;

  const int count = group.dataCount();
  for ( int i = 0; i < count; ++i ) {
    if ( group.data( i ).name().contains( filterString, Qt::CaseInsensitive ) )
      return true;
    if ( group.data( i ).email().contains( filterString, Qt::CaseInsensitive ) )
      return true;
  }

  return false;
}

#include "contactsfilterproxymodel.moc"
