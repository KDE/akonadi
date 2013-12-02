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
    Private()
      : flags( 0 ),
        mExcludeVirtualCollections( false )
    {
    }
    QString mFilter;
    ContactsFilterProxyModel::FilterFlags flags;
    bool mExcludeVirtualCollections;
};

ContactsFilterProxyModel::ContactsFilterProxyModel( QObject *parent )
  : QSortFilterProxyModel( parent ), d( new Private )
{
  // contact names should be sorted correctly
  setSortLocaleAware( true );
  setDynamicSortFilter( true );
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
  const QModelIndex index = sourceModel()->index( row, 0, parent );
  if ( d->mExcludeVirtualCollections ) {
    const Akonadi::Collection collection = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
    if ( collection.isValid() && collection.isVirtual() ) {
      return false;
    }
  }

  if ( ( d->mFilter.isEmpty() ) && ( !( d->flags & ContactsFilterProxyModel::HasEmail ) ) ) {
    return true;
  }

  const Akonadi::Item item = index.data( Akonadi::EntityTreeModel::ItemRole ).value<Akonadi::Item>();

  if ( item.hasPayload<KABC::Addressee>() ) {
    const KABC::Addressee contact = item.payload<KABC::Addressee>();
    if ( d->flags & ContactsFilterProxyModel::HasEmail ) {
      if ( contact.emails().isEmpty() ) {
        return false;
      }
    }
    if ( !d->mFilter.isEmpty() ) {
        return contactMatchesFilter( contact, d->mFilter );
    }
  } else {
    if ( !d->mFilter.isEmpty() ) {
      if ( item.hasPayload<KABC::ContactGroup>() ) {
      const KABC::ContactGroup group = item.payload<KABC::ContactGroup>();
      return contactGroupMatchesFilter( group, d->mFilter );
      }
    }
  }

  return true;
}

bool ContactsFilterProxyModel::lessThan( const QModelIndex &leftIndex, const QModelIndex &rightIndex ) const
{
  const QDate leftDate = leftIndex.data( ContactsTreeModel::DateRole ).toDate();
  const QDate rightDate = rightIndex.data( ContactsTreeModel::DateRole ).toDate();
  if ( leftDate.isValid() && rightDate.isValid() ) {
    if ( leftDate.month() < rightDate.month() ) {
      return true;
    } else if ( leftDate.month() == rightDate.month() ) {
      if ( leftDate.day() < rightDate.day() ) {
        return true;
      }
    } else {
      return false;
    }
  }

  return QSortFilterProxyModel::lessThan( leftIndex, rightIndex );
}

void ContactsFilterProxyModel::setFilterFlags( ContactsFilterProxyModel::FilterFlags flags )
{
  d->flags = flags;
}

void ContactsFilterProxyModel::setExcludeVirtualCollections( bool exclude )
{
  if ( exclude != d->mExcludeVirtualCollections ) {
    d->mExcludeVirtualCollections = exclude;
    invalidateFilter();
  }
}

Qt::ItemFlags ContactsFilterProxyModel::flags( const QModelIndex& index ) const
{
  if ( !index.isValid() ) {
    // Don't crash
    return 0;
  }
  const Akonadi::Collection collection = index.data( Akonadi::EntityTreeModel::CollectionRole ).value<Akonadi::Collection>();
  if ( collection.isValid() ) {
    return QSortFilterProxyModel::flags( index ) & ~( Qt::ItemIsSelectable );
  }
  return QSortFilterProxyModel::flags( index );
}

static bool addressMatchesFilter( const KABC::Address &address, const QString &filterString )
{
  if ( address.street().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( address.locality().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( address.region().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( address.postalCode().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( address.country().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( address.label().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( address.postOfficeBox().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  return false;
}

static bool contactMatchesFilter( const KABC::Addressee &contact, const QString &filterString )
{
  if ( contact.assembledName().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.formattedName().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.nickName().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.birthday().toString().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  const KABC::Address::List addresses = contact.addresses();
  int count = addresses.count();
  for ( int i = 0; i < count; ++i ) {
    if ( addressMatchesFilter( addresses.at( i ), filterString ) ) {
      return true;
    }
  }

  const KABC::PhoneNumber::List phoneNumbers = contact.phoneNumbers();
  count = phoneNumbers.count();
  for ( int i = 0; i < count; ++i ) {
    if ( phoneNumbers.at( i ).number().contains( filterString, Qt::CaseInsensitive ) ) {
      return true;
    }
  }

  const QStringList emails = contact.emails();
  count = emails.count();
  for ( int i = 0; i < count; ++i ) {
    if ( emails.at( i ).contains( filterString, Qt::CaseInsensitive ) ) {
      return true;
    }
  }

  const QStringList categories = contact.categories();
  count = categories.count();
  for ( int i = 0; i < count; ++i ) {
    if ( categories.at( i ).contains( filterString, Qt::CaseInsensitive ) ) {
      return true;
    }
  }

  if ( contact.mailer().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.title().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.role().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.organization().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.department().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.note().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  if ( contact.url().url().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  const QStringList customs = contact.customs();
  count = customs.count();
  for ( int i = 0; i < count; ++i ) {
    if ( customs.at( i ).contains( filterString, Qt::CaseInsensitive ) ) {
      return true;
    }
  }

  return false;
}

bool contactGroupMatchesFilter( const KABC::ContactGroup &group, const QString &filterString )
{
  if ( group.name().contains( filterString, Qt::CaseInsensitive ) ) {
    return true;
  }

  const uint count = group.dataCount();
  for ( uint i = 0; i < count; ++i ) {
    if ( group.data( i ).name().contains( filterString, Qt::CaseInsensitive ) ) {
      return true;
    }
    if ( group.data( i ).email().contains( filterString, Qt::CaseInsensitive ) ) {
      return true;
    }
  }

  return false;
}

