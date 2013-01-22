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

  // by default search for all contacts
  ItemSearchJob::setQuery( QLatin1String( ""
#ifdef AKONADI_USE_STRIGI_SEARCH
                                          "<request>"
                                          "  <query>"
                                          "    <equals>"
                                          "      <field name=\"type\"/>"
                                          "      <string>PersonContact</string>"
                                          "    </equals>"
                                          "  </query>"
                                          "</request>"
#else
                                          "SELECT ?r WHERE { ?r a nco:Contact }"
#endif
                                        ) );
}

ContactSearchJob::~ContactSearchJob()
{
  delete d;
}

void ContactSearchJob::setQuery( Criterion criterion, const QString &value )
{
  setQuery( criterion, value, ExactMatch );
}

// helper method, returns the SPARQL sub-expression to be used for finding
// string either as a whole word, the start of a word, or anywhere in a word
static QString containsQueryString( bool doWholeWordSearch, bool matchWordBoundary )
{
  if ( doWholeWordSearch ) {
    return QString::fromLatin1( "?v bif:contains \"'%1'\" . " );
  } else {
      if ( matchWordBoundary ) {
          return QString::fromLatin1( "?v bif:contains \"'%1*'\" . " );
      } else {
          return QString::fromLatin1( "FILTER regex(str(?v), \"%1\", \"i\")" );
      }
  }
}

void ContactSearchJob::setQuery( Criterion criterion, const QString &value, Match match )
{
  if ( match == StartsWithMatch && value.size() < 4 ) {
    match = ExactMatch;
  }

  const bool doWholeWordSearch = value.size() < 3;
  const bool matchWordBoundary = match == ContainsWordBoundaryMatch;

  QString query;

  if ( match == ExactMatch ) {
    if ( criterion == Name ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <equals>"
          "        <field name=\"fullname\"/>"
          "        <string>%1</string>"
          "      </equals>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "   "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:fullname \"%1\"^^<http://www.w3.org/2001/XMLSchema#string>. "
          "  "
          "} "
#endif
      );
    } else if ( criterion == Email ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <equals>"
          "        <field name=\"emailAddress\"/>"
          "        <string>%1</string>"
          "      </equals>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?person ?reqProp1 "
          "WHERE { "
          "   "
          "    ?person <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?person nco:hasEmailAddress ?email . "
          "    ?email nco:emailAddress \"%1\"^^<http://www.w3.org/2001/XMLSchema#string> . "
          "   "
          "}"
#endif
      );
    } else if ( criterion == NickName ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <equals>"
          "        <field name=\"nickname\"/>"
          "        <string>%1</string>"
          "      </equals>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "   "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:nickname \"%1\"^^<http://www.w3.org/2001/XMLSchema#string> ."
          "  "
          "}"
#endif
      );
    } else if ( criterion == NameOrEmail ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <or>"
          "        <equals>"
          "          <field name=\"fullname\"/>"
          "          <string>%1</string>"
          "        </equals>"
          "        <equals>"
          "          <field name=\"nameGiven\"/>"
          "          <string>%1</string>"
          "        </equals>"
          "        <equals>"
          "          <field name=\"nameFamily\"/>"
          "          <string>%1</string>"
          "        </equals>"
          "        <equals>"
          "          <field name=\"emailAddress\"/>"
          "          <string>%1</string>"
          "        </equals>"
          "      </or>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "   "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    { ?r ?p \"%1\"^^<http://www.w3.org/2001/XMLSchema#string>. "
          "      FILTER(?p in (nco:fullname, nco:nameGiven, nco:nameFamily)) . }"
          "    UNION "
          "    { ?r nco:hasEmailAddress ?email . "
          "      ?email nco:emailAddress \"%1\"^^<http://www.w3.org/2001/XMLSchema#string> . } "
          "  "
          "}"
#endif
      );
    } else if ( criterion == ContactUid ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <equals>"
          "        <field name=\"contactUID\"/>"
          "        <string>%1</string>"
          "      </equals>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "   "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:contactUID \"%1\"^^<http://www.w3.org/2001/XMLSchema#string> ."
          "   "
          "}"
#endif
      );
    }
  } else if ( match == StartsWithMatch ) {
    if ( criterion == Name ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <startsWith>"
          "        <field name=\"fullname\"/>"
          "        <string>%1</string>"
          "      </startsWith>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "   "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:fullname ?v . "
          "    ?v bif:contains \"'%1*'\" . "
          "  "
          "} "
#endif
      );
    } else if ( criterion == Email ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <startsWith>"
          "        <field name=\"emailAddress\"/>"
          "        <string>%1</string>"
          "      </startsWith>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?person ?reqProp1 "
          "WHERE { "
          "   "
          "    ?person <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?person nco:hasEmailAddress ?email . "
          "    ?email nco:emailAddress ?v . "
          "    ?v bif:contains \"'%1\'\" . "
          "  "
          "}"
#endif
      );
    } else if ( criterion == NickName ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <startsWith>"
          "        <field name=\"nickname\"/>"
          "        <string>%1</string>"
          "      </startsWith>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "   "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:nickname ?v . "
          "    ?v bif:contains \"'%1\'\" . "
          "  "
          "}"
#endif
      );
    } else if ( criterion == NameOrEmail ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <or>"
          "        <startsWith>"
          "          <field name=\"fullname\"/>"
          "          <string>%1</string>"
          "        </startsWith>"
          "        <startsWith>"
          "          <field name=\"nameGiven\"/>"
          "          <string>%1</string>"
          "        </startsWith>"
          "        <startsWith>"
          "          <field name=\"nameFamily\"/>"
          "          <string>%1</string>"
          "        </startsWith>"
          "        <startsWith>"
          "          <field name=\"emailAddress\"/>"
          "          <string>%1</string>"
          "        </startsWith>"
          "      </or>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "   "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    { ?r ?p ?v . "
          "      FILTER(?p in (nco:fullname, nco:nameGiven, nco:nameFamily)). "
          "      ?v bif:contains \"'%1'\" . }"
          "    UNION "
          "    { ?r nco:hasEmailAddress ?email . "
          "      ?email nco:emailAddress ?v . "
          "      ?v bif:contains \"'%1'\" . }"
          "  "
          "}"
#endif
      );
    } else if ( criterion == ContactUid ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <startsWith>"
          "        <field name=\"contactUID\"/>"
          "        <string>%1</string>"
          "      </startsWith>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "  "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:contactUID ?v . "
          "    ?v bif:contains \"'%1*'\" . "
          " "
          "}"
#endif
      );
    }
  } else if ( match == ContainsMatch || match == ContainsWordBoundaryMatch ) {
    if ( criterion == Name ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <contains>"
          "        <field name=\"fullname\"/>"
          "        <string>%1</string>"
          "      </contains>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "  "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:fullname ?v . "
          "%1"
          "  "
          "} "
#endif
      );
      query = query.arg( containsQueryString( doWholeWordSearch, matchWordBoundary ) );
    } else if ( criterion == Email ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <contains>"
          "        <field name=\"emailAddress\"/>"
          "        <string>%1</string>"
          "      </contains>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?person ?reqProp1 "
          "WHERE { "
          "  "
          "    ?person <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?person nco:hasEmailAddress ?email . "
          "    ?email nco:emailAddress ?v . "
          "%1"
          "  "
          "}"
#endif
      );
      query = query.arg( containsQueryString( doWholeWordSearch, matchWordBoundary ) );
    } else if ( criterion == NickName ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <contains>"
          "        <field name=\"nickname\"/>"
          "        <string>%1</string>"
          "      </contains>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "  "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:nickname ?v . "
          "%1"
          "  "
          "}"
#endif
      );
      query = query.arg( containsQueryString( doWholeWordSearch, matchWordBoundary ) );
    } else if ( criterion == NameOrEmail ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>PersonContact</string>"
          "      </equals>"
          "      <or>"
          "        <contains>"
          "          <field name=\"fullname\"/>"
          "          <string>%1</string>"
          "        </contains>"
          "        <contains>"
          "          <field name=\"nameGiven\"/>"
          "          <string>%1</string>"
          "        </contains>"
          "        <contains>"
          "          <field name=\"nameFamily\"/>"
          "          <string>%1</string>"
          "        </contains>"
          "        <contains>"
          "          <field name=\"emailAddress\"/>"
          "          <string>%1</string>"
          "        </contains>"
          "      </or>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "  "
          "    ?r<" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    { ?r ?p ?v ."
          "      FILTER(?p in (nco:fullname, nco:nameGiven, nco:nameFamily) ) ."
          "%1 } UNION"
          "    { ?r nco:hasEmailAddress ?email . "
          "      ?email nco:emailAddress ?v . "
          "%1 }"
          " "
          "}"
#endif
      );
      query = query.arg( containsQueryString( doWholeWordSearch, matchWordBoundary ) );
    } else if ( criterion == ContactUid ) {
      query += QString::fromLatin1(
#ifdef AKONADI_USE_STRIGI_SEARCH
          "<request>"
          "  <query>"
          "    <and>"
          "      <equals>"
          "        <field name=\"type\"/>"
          "        <string>Contact</string>"
          "      </equals>"
          "      <contains>"
          "        <field name=\"contactUID\"/>"
          "        <string>%1</string>"
          "      </contains>"
          "    </and>"
          "  </query>"
          "</request>"
#else
          "SELECT DISTINCT ?r ?reqProp1 "
          "WHERE { "
          "  "
          "    ?r <" + akonadiItemIdUri().toEncoded() + "> ?reqProp1 . "
          "    ?r nco:contactUID ?v . "
          "    ?v bif:contains \"'%1'\" . "
          " "
          "}"
#endif
      );
    }
  }

  if ( d->mLimit != -1 ) {
#ifndef AKONADI_USE_STRIGI_SEARCH
    query += QString::fromLatin1( " LIMIT %1" ).arg( d->mLimit );
#endif
  }
  query = query.arg( value );

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

