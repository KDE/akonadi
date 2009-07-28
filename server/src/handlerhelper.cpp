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

#include "../libs/imapparser_p.h"

using namespace Akonadi;

QByteArray Akonadi::HandlerHelper::normalizeCollectionName(const QByteArray &name)
{
  QByteArray collectionByteArray = name;
  if ( collectionByteArray.startsWith( '/' )  )
    collectionByteArray = collectionByteArray.right( collectionByteArray.length() - 1 );
  if ( collectionByteArray.endsWith( '/' ) )
    collectionByteArray = collectionByteArray.left( collectionByteArray.length() - 1 );
  return collectionByteArray;
}

Collection HandlerHelper::collectionFromIdOrName(const QByteArray & id)
{
  // id is a number
  bool ok = false;
  qint64 collectionId = id.toLongLong( &ok );
  if ( ok )
    return Collection::retrieveById( collectionId );

  // id is a path
  QString path = QString::fromUtf8( normalizeCollectionName( id ) ); // ### should be UTF-7 for real IMAP compatibility

  const QStringList pathParts = path.split( QLatin1Char('/') );
  Collection col;
  foreach ( const QString &part, pathParts ) {
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition( Collection::nameColumn(), Query::Equals, part );
    if ( col.isValid() )
      qb.addValueCondition( Collection::parentIdColumn(), Query::Equals, col.id() );
    else
      qb.addValueCondition( Collection::parentIdColumn(), Query::Equals, 0 );
    if ( !qb.exec() )
      return Collection();
    Collection::List list = qb.result();
    if ( list.count() != 1 )
      return Collection();
    col = list.first();
  }
  return col;
}

QString HandlerHelper::pathForCollection(const Collection & col)
{
  QStringList parts;
  Collection current = col;
  while ( current.isValid() ) {
    parts.prepend( QString::fromUtf8( current.name() ) );
    current = current.parent();
  }
  return parts.join( QLatin1String("/") );
}

int HandlerHelper::itemCount(const Collection & col)
{
  CountQueryBuilder qb;
  qb.addTable( PimItem::tableName() );
  qb.addValueCondition( PimItem::collectionIdColumn(), Query::Equals, col.id() );
  if ( !qb.exec() )
    return -1;
  return qb.result();
}

int HandlerHelper::itemWithFlagCount(const Collection & col, const QString & flag)
{
  CountQueryBuilder qb;
  qb.addTable( PimItem::tableName() );
  qb.addTable( Flag::tableName() );
  qb.addTable( PimItemFlagRelation::tableName() );
  qb.addValueCondition( PimItem::collectionIdFullColumnName(), Query::Equals, col.id() );
  qb.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, PimItemFlagRelation::leftFullColumnName() );
  qb.addColumnCondition( Flag::idFullColumnName(), Query::Equals, PimItemFlagRelation::rightFullColumnName() );
  qb.addValueCondition( Flag::nameFullColumnName(), Query::Equals, flag );
  if ( !qb.exec() )
    return -1;
  return qb.result();
}

int HandlerHelper::itemWithoutFlagCount(const Collection & col, const QString & flag)
{
  // FIXME optimize me: use only one query or reuse previously done count
  const int flagCount = itemWithFlagCount( col, flag );
  const int totalCount = itemCount( col );
  if ( totalCount < 0 || flagCount < 0 )
    return -1;
  return totalCount - flagCount;
}

qint64 HandlerHelper::itemsTotalSize(const Collection & col)
{
  QueryBuilder qb;
  qb.addTable( PimItem::tableName() );
  qb.addValueCondition( PimItem::collectionIdColumn(), Query::Equals, col.id() );
  qb.addColumn( QLatin1String("sum(size)") );

  if ( !qb.exec() )
    return -1;
  if ( !qb.query().next() ) {
      qDebug() << "Error during retrieving result of query:" << qb.query().lastError().text();
      return -1;
  }
  return qb.query().value( 0 ).toLongLong();
}

int HandlerHelper::parseCachePolicy(const QByteArray & data, Collection & col, int start, bool *changed )
{
  bool inheritChanged = false;
  bool somethingElseChanged = false;

  QList<QByteArray> params;
  int end = Akonadi::ImapParser::parseParenthesizedList( data, params, start );
  for ( int i = 0; i < params.count() - 1; i += 2 ) {
    const QByteArray key = params[i];
    const QByteArray value = params[i + 1];

    if ( key == "INHERIT" ) {
      const bool inherit = value == "true";
      inheritChanged = col.cachePolicyInherit() != inherit;
      col.setCachePolicyInherit( inherit );
    } else if ( key == "INTERVAL" ) {
      const int interval = value.toInt();
      somethingElseChanged = somethingElseChanged || interval != col.cachePolicyCheckInterval();
      col.setCachePolicyCheckInterval( interval );
    } else if ( key == "CACHETIMEOUT" ) {
      const int timeout = value.toInt();
      somethingElseChanged = somethingElseChanged || timeout != col.cachePolicyCacheTimeout();
      col.setCachePolicyCacheTimeout( timeout );
    } else if ( key == "SYNCONDEMAND" ) {
      const bool syncOnDemand = value == "true";
      somethingElseChanged = somethingElseChanged || syncOnDemand != col.cachePolicySyncOnDemand();
      col.setCachePolicySyncOnDemand( syncOnDemand );
    } else if ( key == "LOCALPARTS" ) {
      QList<QByteArray> tmp;
      QStringList partsList;
      Akonadi::ImapParser::parseParenthesizedList( value, tmp );
      foreach ( const QByteArray &ba, tmp )
        partsList << QString::fromLatin1( ba );
      const QString parts = partsList.join( QLatin1String( " " ) );
      somethingElseChanged = somethingElseChanged || col.cachePolicyLocalParts() != parts;
      col.setCachePolicyLocalParts( parts );
    }
  }

  if ( changed && (inheritChanged || (!col.cachePolicyInherit() && somethingElseChanged)) )
    *changed = true;

  return end;
}

QByteArray HandlerHelper::cachePolicyToByteArray(const Collection & col)
{
  QByteArray rv = "CACHEPOLICY (";
  rv += "INHERIT " + ( col.cachePolicyInherit() ? QByteArray("true") : QByteArray("false") );
  rv += " INTERVAL " + QByteArray::number( col.cachePolicyCheckInterval() );
  rv += " CACHETIMEOUT " + QByteArray::number( col.cachePolicyCacheTimeout() );
  rv += " SYNCONDEMAND " + ( col.cachePolicySyncOnDemand() ? QByteArray("true") : QByteArray("false") );
  rv += " LOCALPARTS (" + col.cachePolicyLocalParts().toLatin1() + ')';
  rv += ')';
  return rv;
}

QByteArray HandlerHelper::collectionToByteArray( const Collection & col, bool hidden, bool includeStatistics )
{
  QByteArray b = QByteArray::number( col.id() ) + ' '
               + QByteArray::number( col.parentId() ) + " (";

  // FIXME: escape " and "\"
  b += "NAME \"" + col.name() + "\" ";
  if ( hidden )
    b+= "MIMETYPE () ";
  else
    b += "MIMETYPE (" + MimeType::joinByName( col.mimeTypes(), QLatin1String( " " ) ).toLatin1() + ") ";
  b += "REMOTEID \"" + col.remoteId().toUtf8() + "\" ";
  b += "RESOURCE \"" + col.resource().name().toUtf8() + "\" ";

  if ( includeStatistics ) {
      b += "MESSAGES " + QByteArray::number( HandlerHelper::itemCount( col ) ) + ' ';
      b += "UNSEEN " + QByteArray::number( HandlerHelper::itemWithoutFlagCount( col, QLatin1String( "\\Seen" ) ) ) + ' ';
      b += "SIZE " + QByteArray::number( HandlerHelper::itemsTotalSize( col ) ) + ' ';
  }

  b += HandlerHelper::cachePolicyToByteArray( col ) + ' ';

  const CollectionAttribute::List attrs = col.attributes();
  for ( int i = 0; i < attrs.size(); ++i ) {
    const CollectionAttribute &attr = attrs[i];
    b += attr.type() + ' ' + ImapParser::quote( attr.value() );
    if ( i != attrs.size() - 1 )
      b += ' ';
  }
  b+= ')';

  return b;
}
