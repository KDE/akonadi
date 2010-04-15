/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#include "persistentsearchattribute.h"

#include <akonadi/private/imapparser_p.h>

#include <QtCore/QString>

using namespace Akonadi;

class PersistentSearchAttribute::Private
{
  public:
    QString queryLanguage;
    QString queryString;
};

PersistentSearchAttribute::PersistentSearchAttribute()
  : d( new Private )
{
}

PersistentSearchAttribute::~PersistentSearchAttribute()
{
  delete d;
}

QString PersistentSearchAttribute::queryLanguage() const
{
  return d->queryLanguage;
}

void PersistentSearchAttribute::setQueryLanguage(const QString& language)
{
  d->queryLanguage = language;
}

QString PersistentSearchAttribute::queryString() const
{
  return d->queryString;
}

void PersistentSearchAttribute::setQueryString(const QString& query)
{
  d->queryString = query;
}

QByteArray PersistentSearchAttribute::type() const
{
  return "PERSISTENTSEARCH"; // ### should eventually be AKONADI_PARAM_PERSISTENTSEARCH
}

Attribute* PersistentSearchAttribute::clone() const
{
  PersistentSearchAttribute* attr = new PersistentSearchAttribute;
  attr->setQueryLanguage( queryLanguage() );
  attr->setQueryString( queryString() );
  return attr;
}

QByteArray PersistentSearchAttribute::serialized() const
{
  QList<QByteArray> l;
  // ### eventually replace with the AKONADI_PARAM_PERSISTENTSEARCH_XXX constants
  l.append( "QUERYLANGUAGE" );
  l.append( d->queryLanguage.toLatin1() );
  l.append( "QUERYSTRING" );
  l.append( d->queryString.toUtf8() );
  return ImapParser::join( l, " " );
}

void PersistentSearchAttribute::deserialize(const QByteArray& data)
{
  QList<QByteArray> l;
  ImapParser::parseParenthesizedList( data, l );
  for ( int i = 0; i < l.size() - 1; i += 2 ) {
    const QByteArray key = l.at( i );
    if ( key == "QUERYLANGUAGE" )
      d->queryLanguage = QString::fromLatin1( l.at( i ) );
    else if ( key == "QUERYSTRING" )
      d->queryString = QString::fromUtf8( l.at( i ) );
  }
}
