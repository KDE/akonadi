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

ContactGroupSearchJob::ContactGroupSearchJob( QObject * parent )
  : ItemSearchJob( QString(), parent ), d( 0 )
{
  fetchScope().fetchFullPayload();

  // by default search for all contact groups
  ItemSearchJob::setQuery( QLatin1String( ""
                                          "prefix nco:<http://www.semanticdesktop.org/ontologies/2007/03/22/nco#>"
                                          "SELECT ?r WHERE { ?r a nco:ContactGroup }" ) );
}

ContactGroupSearchJob::~ContactGroupSearchJob()
{
}

void ContactGroupSearchJob::setQuery( Criterion criterion, const QString &value )
{
  QString query;

  if ( criterion == Name ) {
    query = QString::fromLatin1( ""
                                 "prefix nco:<http://www.semanticdesktop.org/ontologies/2007/03/22/nco#>"
                                 "SELECT ?group WHERE {"
                                 "  ?group nco:contactGroupName \"%1\"^^<http://www.w3.org/2001/XMLSchema#string>."
                                 "}" );
  }

  query = query.arg( value );

  ItemSearchJob::setQuery( query );
}

KABC::ContactGroup::List ContactGroupSearchJob::contactGroups() const
{
  KABC::ContactGroup::List contactGroups;

  foreach ( const Item &item, items() ) {
    if ( item.hasPayload<KABC::ContactGroup>() )
      contactGroups.append( item.payload<KABC::ContactGroup>() );
  }

  return contactGroups;
}

#include "contactgroupsearchjob.moc"
