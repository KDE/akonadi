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

#include "contactgroupsearchjob.h"

#include <searchquery.h>
#include <akonadi/itemfetchscope.h>
#include <QStringList>

using namespace Akonadi;

class ContactGroupSearchJob::Private
{
  public:
    int mLimit;
};

ContactGroupSearchJob::ContactGroupSearchJob( QObject * parent )
  : ItemSearchJob( QString(), parent ), d( new Private )
{
  fetchScope().fetchFullPayload();
  d->mLimit = -1;

  setMimeTypes( QStringList() << KABC::ContactGroup::mimeType() );

  // by default search for all contact groups
  Akonadi::SearchQuery query;
  query.addTerm( ContactSearchTerm(ContactSearchTerm::All, QVariant(), SearchTerm::CondEqual) );
  ItemSearchJob::setQuery( query );
}

ContactGroupSearchJob::~ContactGroupSearchJob()
{
  delete d;
}

void ContactGroupSearchJob::setQuery( Criterion criterion, const QString &value )
{
  // Exact match was the default in 4.4, so we have to keep it and ContactSearchJob has something
  // else as default
  setQuery( criterion, value, ExactMatch );
}

static Akonadi::SearchTerm::Condition matchType( ContactGroupSearchJob::Match match )
{
  switch( match ) {
    case ContactGroupSearchJob::ExactMatch:
      return Akonadi::SearchTerm::CondEqual;
    case ContactGroupSearchJob::StartsWithMatch:
    case ContactGroupSearchJob::ContainsMatch:
      return Akonadi::SearchTerm::CondContains;
  }
  return Akonadi::SearchTerm::CondEqual;
}

void ContactGroupSearchJob::setQuery( Criterion criterion, const QString &value, Match match )
{
  Akonadi::SearchQuery query;
  if ( criterion == Name ) {
    query.addTerm(ContactSearchTerm(ContactSearchTerm::Name, value, matchType(match)));
  }

  query.setLimit( d->mLimit );

  ItemSearchJob::setQuery( query );
}

void ContactGroupSearchJob::setLimit( int limit )
{
  d->mLimit = limit;
}

KABC::ContactGroup::List ContactGroupSearchJob::contactGroups() const
{
  KABC::ContactGroup::List contactGroups;

  foreach ( const Item &item, items() ) {
    if ( item.hasPayload<KABC::ContactGroup>() ) {
      contactGroups.append( item.payload<KABC::ContactGroup>() );
    }
  }

  return contactGroups;
}

