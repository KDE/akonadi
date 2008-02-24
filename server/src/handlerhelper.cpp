/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "handlerhelper.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"

#include <libakonadi/imapparser.h>

using namespace Akonadi;

QString Akonadi::HandlerHelper::normalizeCollectionName(const QString &name)
{
  QString collection = name;
  if ( collection.startsWith( QLatin1Char('/') )  )
    collection = collection.right( collection.length() - 1 );
  if ( collection.endsWith( QLatin1Char('/') ) )
    collection = collection.left( collection.length() - 1 );
  return collection;
}

Location HandlerHelper::collectionFromIdOrName(const QByteArray & id)
{
  // id is a number
  bool ok = false;
  int collectionId = id.toInt( &ok );
  if ( ok )
    return Location::retrieveById( collectionId );

  // id is a path
  QString path = QString::fromUtf8( id ); // ### should be UTF-7 for real IMAP compatibility
  path = normalizeCollectionName( path );

  const QStringList pathParts = path.split( QLatin1Char('/') );
  Location loc;
  foreach ( const QString part, pathParts ) {
    SelectQueryBuilder<Location> qb;
    qb.addValueCondition( Location::nameColumn(), Query::Equals, part );
    if ( loc.isValid() )
      qb.addValueCondition( Location::parentIdColumn(), Query::Equals, loc.id() );
    else
      qb.addValueCondition( Location::parentIdColumn(), Query::Equals, 0 );
    if ( !qb.exec() )
      return Location();
    Location::List list = qb.result();
    if ( list.count() != 1 )
      return Location();
    loc = list.first();
  }
  return loc;
}

QString HandlerHelper::pathForCollection(const Location & loc)
{
  QStringList parts;
  Location current = loc;
  while ( current.isValid() ) {
    parts.prepend( current.name() );
    current = current.parent();
  }
  return parts.join( QLatin1String("/") );
}

int HandlerHelper::itemCount(const Location & loc)
{
  CountQueryBuilder qb;
  qb.addTable( PimItem::tableName() );
  qb.addValueCondition( PimItem::locationIdColumn(), Query::Equals, loc.id() );
  if ( !qb.exec() )
    return -1;
  return qb.result();
}

int HandlerHelper::itemWithFlagCount(const Location & loc, const QString & flag)
{
  CountQueryBuilder qb;
  qb.addTable( PimItem::tableName() );
  qb.addTable( Flag::tableName() );
  qb.addTable( PimItemFlagRelation::tableName() );
  qb.addValueCondition( PimItem::locationIdFullColumnName(), Query::Equals, loc.id() );
  qb.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, PimItemFlagRelation::leftFullColumnName() );
  qb.addColumnCondition( Flag::idFullColumnName(), Query::Equals, PimItemFlagRelation::rightFullColumnName() );
  qb.addValueCondition( Flag::nameFullColumnName(), Query::Equals, flag );
  if ( !qb.exec() )
    return -1;
  return qb.result();
}

int HandlerHelper::itemWithoutFlagCount(const Location & loc, const QString & flag)
{
  // FIXME optimize me: use only one query or reuse previously done count
  const int flagCount = itemWithFlagCount( loc, flag );
  const int totalCount = itemCount( loc );
  if ( totalCount < 0 || flagCount < 0 )
    return -1;
  return totalCount - flagCount;
}

int HandlerHelper::parseCachePolicy(const QByteArray & data, Location & loc, int start)
{
  QList<QByteArray> params;
  int end = Akonadi::ImapParser::parseParenthesizedList( data, params, start );
  for ( int i = 0; i < params.count() - 1; i += 2 ) {
    const QByteArray key = params[i];
    const QByteArray value = params[i + 1];

    if ( key == "INHERIT" )
      loc.setCachePolicyInherit( value == "true" );
    else if ( key == "INTERVAL" )
      loc.setCachePolicyCheckInterval( value.toInt() );
    else if ( key == "CACHETIMEOUT" )
      loc.setCachePolicyCacheTimeout( value.toInt() );
    else if ( key == "SYNCONDEMAND" )
      loc.setCachePolicySyncOnDemand( value == "true" );
    else if ( key == "LOCALPARTS" ) {
      QList<QByteArray> tmp;
      QStringList parts;
      Akonadi::ImapParser::parseParenthesizedList( value, tmp );
      foreach ( const QByteArray ba, tmp )
        parts << QString::fromLatin1( ba );
      loc.setCachePolicyLocalParts( parts.join( QLatin1String(" ") ) );
    }
  }
  return end;
}

QByteArray HandlerHelper::cachePolicyToByteArray(const Location & loc)
{
  QByteArray rv = "CACHEPOLICY (";
  rv += "INHERIT " + ( loc.cachePolicyInherit() ? QByteArray("true") : QByteArray("false") );
  rv += " INTERVAL " + QByteArray::number( loc.cachePolicyCheckInterval() );
  rv += " CACHETIMEOUT " + QByteArray::number( loc.cachePolicyCacheTimeout() );
  rv += " SYNCONDEMAND " + ( loc.cachePolicySyncOnDemand() ? QByteArray("true") : QByteArray("false") );
  rv += " LOCALPARTS (" + loc.cachePolicyLocalParts().toLatin1() + ")";
  rv += ")";
  return rv;
}

QByteArray HandlerHelper::collectionToByteArray( const Location & loc, bool hidden )
{
  QByteArray b = QByteArray::number( loc.id() ) + ' '
               + QByteArray::number( loc.parentId() ) + " (";

  // FIXME: escape " and "\"
  b += "NAME \"" + loc.name().toUtf8() + "\" ";
  if ( hidden )
    b+= "MIMETYPE () ";
  else
    b += "MIMETYPE (" + MimeType::joinByName( loc.mimeTypes(), QLatin1String( " " ) ).toLatin1() + ") ";
  b += "REMOTEID \"" + loc.remoteId().toUtf8() + "\" ";
  b += "RESOURCE \"" + loc.resource().name().toUtf8() + "\" ";

  b += HandlerHelper::cachePolicyToByteArray( loc ) + ' ';

  LocationAttribute::List attrs = loc.attributes();
  foreach ( const LocationAttribute attr, attrs )
    b += attr.type() + ' ' + ImapParser::quote( attr.value() );
  b+= ')';

  return b;
}
