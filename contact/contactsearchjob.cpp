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

#include "contactsearchjob.h"
#include <searchquery.h>

#include <akonadi/itemfetchscope.h>

using namespace Akonadi;

class ContactSearchJob::Private
{
  public:
    int mLimit;
};

ContactSearchJob::ContactSearchJob( QObject * parent )
  : ItemSearchJob( QString(), parent ), d( new Private() )
{
  fetchScope().fetchFullPayload();
  d->mLimit = -1;

  setMimeTypes( QStringList() << KABC::Addressee::mimeType() );

  // by default search for all contacts
  Akonadi::SearchQuery query;
  query.addTerm( ContactSearchTerm(ContactSearchTerm::All, QVariant(), SearchTerm::CondEqual) );
  ItemSearchJob::setQuery( query );
}

ContactSearchJob::~ContactSearchJob()
{
  delete d;
}

void ContactSearchJob::setQuery( Criterion criterion, const QString &value )
{
  setQuery( criterion, value, ExactMatch );
}

static Akonadi::SearchTerm::Condition matchType( ContactSearchJob::Match match )
{
  switch ( match ) {
    case ContactSearchJob::ExactMatch:
      return Akonadi::SearchTerm::CondEqual;
    case ContactSearchJob::StartsWithMatch:
    case ContactSearchJob::ContainsWordBoundaryMatch:
    case ContactSearchJob::ContainsMatch:
      return Akonadi::SearchTerm::CondContains;
  }
  return Akonadi::SearchTerm::CondEqual;
}

void ContactSearchJob::setQuery( Criterion criterion, const QString &value, Match match )
{
 Akonadi::SearchQuery query( SearchTerm::RelOr) ;

  if ( criterion == Name ) {
    query.addTerm( ContactSearchTerm( ContactSearchTerm::Name, value, matchType( match ) ) );
  } else if ( criterion == Email ) {
    query.addTerm( ContactSearchTerm( ContactSearchTerm::Email, value, matchType( match ) ) );
  } else if ( criterion == NickName ) {
    query.addTerm( ContactSearchTerm( ContactSearchTerm::Nickname, value, matchType( match ) ) );
  } else if ( criterion == NameOrEmail ) {
    query.addTerm( ContactSearchTerm( ContactSearchTerm::Name, value, matchType( match ) ) );
    query.addTerm( ContactSearchTerm(ContactSearchTerm::Email, value, matchType( match ) ) );
  } else if ( criterion == ContactUid ) {
    query.addTerm( ContactSearchTerm( ContactSearchTerm::Uid, value, matchType( match ) ) );
  }

  query.setLimit( d->mLimit );

  ItemSearchJob::setQuery( query );
}

void ContactSearchJob::setLimit( int limit )
{
  d->mLimit = limit;
}

KABC::Addressee::List ContactSearchJob::contacts() const
{
  KABC::Addressee::List contacts;

  foreach ( const Item &item, items() ) {
    if ( item.hasPayload<KABC::Addressee>() ) {
      contacts.append( item.payload<KABC::Addressee>() );
    }
  }

  return contacts;
}

