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

#include <akonadi/itemfetchscope.h>

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

  // by default search for all contact groups
  ItemSearchJob::setQuery( QLatin1String( "SELECT ?r WHERE { ?r a nco:ContactGroup }" ) );
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

void ContactGroupSearchJob::setQuery( Criterion criterion, const QString &value, Match match )
{
  QString query;

  if ( match == ExactMatch ) {
    if ( criterion == Name ) {
      query += QString::fromLatin1(
        QByteArray(QByteArray("SELECT DISTINCT ?group "
        "WHERE { "
        "  graph ?g { "
        "    ?group <") + akonadiItemIdUri().toEncoded() + QByteArray("> ?itemId . "
        "    ?group nco:contactGroupName \"%1\"^^<http://www.w3.org/2001/XMLSchema#string>."
        "  } "
        "}"))
      );
    }
  } else if ( match == ContainsMatch ) {
    if ( criterion == Name ) {
      query += QString::fromLatin1(
        QByteArray(QByteArray("SELECT DISTINCT ?group "
        "WHERE { "
        "  graph ?g { "
        "    ?group <") + akonadiItemIdUri().toEncoded() + QByteArray("> ?itemId . "
        "    ?group nco:contactGroupName ?v . "
        "    ?v bif:contains \"'%1'\""
        "  } "
        "}"))
      );
    }
  } else if ( match == StartsWithMatch ) {
    if ( criterion == Name ) {
      query += QString::fromLatin1(
        QByteArray(QByteArray("SELECT DISTINCT ?group "
        "WHERE { "
        "  graph ?g { "
        "    ?group <") + akonadiItemIdUri().toEncoded() + QByteArray("> ?itemId . "
        "    ?group nco:contactGroupName ?v . "
        "    ?v bif:contains \"'%1*'\""
        "  } "
        "}"))
      );
    }
  }

  if ( d->mLimit != -1 ) {
    query += QString::fromLatin1( " LIMIT %1" ).arg( d->mLimit );
  }

  query = query.arg( value );

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

