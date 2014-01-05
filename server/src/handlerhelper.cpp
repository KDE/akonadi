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
#include "imapstreamparser.h"
#include "storage/countquerybuilder.h"
#include "storage/datastore.h"
#include "storage/selectquerybuilder.h"
#include "libs/imapparser_p.h"
#include "libs/protocol_p.h"
#include "handler.h"

#include <QtSql/QSqlError>

using namespace Akonadi;

Collection HandlerHelper::collectionFromIdOrName( const QByteArray &id )
{
  // id is a number
  bool ok = false;
  qint64 collectionId = id.toLongLong( &ok );
  if ( ok ) {
    return Collection::retrieveById( collectionId );
  }

  // id is a path
  QString path = QString::fromUtf8( id ); // ### should be UTF-7 for real IMAP compatibility

  const QStringList pathParts = path.split( QLatin1Char( '/' ), QString::SkipEmptyParts );
  Collection col;
  Q_FOREACH ( const QString &part, pathParts ) {
    SelectQueryBuilder<Collection> qb;
    qb.addValueCondition( Collection::nameColumn(), Query::Equals, part );
    if ( col.isValid() ) {
      qb.addValueCondition( Collection::parentIdColumn(), Query::Equals, col.id() );
    } else {
      qb.addValueCondition( Collection::parentIdColumn(), Query::Is, QVariant() );
    }
    if ( !qb.exec() ) {
      return Collection();
    }
    Collection::List list = qb.result();
    if ( list.count() != 1 ) {
      return Collection();
    }
    col = list.first();
  }
  return col;
}

QString HandlerHelper::pathForCollection( const Collection &col )
{
  QStringList parts;
  Collection current = col;
  while ( current.isValid() ) {
    parts.prepend( current.name() );
    current = current.parent();
  }
  return parts.join( QLatin1String( "/" ) );
}

bool HandlerHelper::itemStatistics( const Akonadi::Collection &col, qint64 &count, qint64 &size )
{
  QueryBuilder qb( PimItem::tableName() );
  qb.addAggregation( PimItem::idColumn(), QLatin1String( "count" ) );
  qb.addAggregation( PimItem::sizeColumn(), QLatin1String( "sum" ) );

  if ( col.resource().isVirtual() ) {
    qb.addJoin( QueryBuilder::InnerJoin, CollectionPimItemRelation::tableName(),
                CollectionPimItemRelation::rightFullColumnName(), PimItem::idFullColumnName() );
    qb.addValueCondition( CollectionPimItemRelation::leftFullColumnName(), Query::Equals, col.id() );
  } else {
    qb.addValueCondition( PimItem::collectionIdColumn(), Query::Equals, col.id() );
  }

  if ( !qb.exec() ) {
    return false;
  }
  if ( !qb.query().next() ) {
    akError() << "Error during retrieving result of statistics query:" << qb.query().lastError().text();
    return false;
  }
  count = qb.query().value( 0 ).toLongLong();
  size = qb.query().value( 1 ).toLongLong();
  return true;
}

int HandlerHelper::itemWithFlagsCount( const Akonadi::Collection &col, const QStringList &flags )
{
  CountQueryBuilder qb( PimItem::tableName(), PimItem::idFullColumnName(), CountQueryBuilder::Distinct );
  qb.addJoin( QueryBuilder::InnerJoin, PimItemFlagRelation::tableName(),
              PimItem::idFullColumnName(), PimItemFlagRelation::leftFullColumnName() );
  if ( col.isVirtual() ) {
    qb.addJoin( QueryBuilder::InnerJoin, CollectionPimItemRelation::tableName(),
                CollectionPimItemRelation::rightFullColumnName(), PimItem::idFullColumnName() );
    qb.addValueCondition( CollectionPimItemRelation::leftFullColumnName(), Query::Equals, col.id() );
  } else {
    qb.addValueCondition( PimItem::collectionIdFullColumnName(), Query::Equals, col.id() );
  }
  Query::Condition cond( Query::Or );
  // We use the below instead of an inner join in the query above because postgres seems
  // to struggle to optimize the two inner joins, despite having indices that should
  // facilitate that. This exploits the fact that the Flag::retrieveByName is fast because
  // it hits an in-memory cache.
  Q_FOREACH ( const QString &flag, flags ) {
    const Flag f = Flag::retrieveByName( flag );
    cond.addValueCondition( PimItemFlagRelation::rightFullColumnName(), Query::Equals, f.id() );
  }
  qb.addCondition( cond );
  if ( !qb.exec() ) {
    return -1;
  }
  return qb.result();
}

int HandlerHelper::itemCount( const Collection &col )
{
  CountQueryBuilder qb( PimItem::tableName() );
  qb.addValueCondition( PimItem::collectionIdColumn(), Query::Equals, col.id() );
  if ( !qb.exec() ) {
    return -1;
  }
  return qb.result();
}

int HandlerHelper::parseCachePolicy( const QByteArray &data, Collection &col, int start, bool *changed )
{
  bool inheritChanged = false;
  bool somethingElseChanged = false;

  QList<QByteArray> params;
  int end = Akonadi::ImapParser::parseParenthesizedList( data, params, start );
  for ( int i = 0; i < params.count() - 1; i += 2 ) {
    const QByteArray key = params[i];
    const QByteArray value = params[i + 1];

    if ( key == AKONADI_PARAM_INHERIT ) {
      const bool inherit = value == "true";
      inheritChanged = col.cachePolicyInherit() != inherit;
      col.setCachePolicyInherit( inherit );
    } else if ( key == AKONADI_PARAM_INTERVAL ) {
      const int interval = value.toInt();
      somethingElseChanged = somethingElseChanged || interval != col.cachePolicyCheckInterval();
      col.setCachePolicyCheckInterval( interval );
    } else if ( key == AKONADI_PARAM_CACHETIMEOUT ) {
      const int timeout = value.toInt();
      somethingElseChanged = somethingElseChanged || timeout != col.cachePolicyCacheTimeout();
      col.setCachePolicyCacheTimeout( timeout );
    } else if ( key == AKONADI_PARAM_SYNCONDEMAND ) {
      const bool syncOnDemand = value == "true";
      somethingElseChanged = somethingElseChanged || syncOnDemand != col.cachePolicySyncOnDemand();
      col.setCachePolicySyncOnDemand( syncOnDemand );
    } else if ( key == AKONADI_PARAM_LOCALPARTS ) {
      QList<QByteArray> tmp;
      QStringList partsList;
      Akonadi::ImapParser::parseParenthesizedList( value, tmp );
      Q_FOREACH ( const QByteArray &ba, tmp ) {
        partsList << QString::fromLatin1( ba );
      }
      const QString parts = partsList.join( QLatin1String( " " ) );
      somethingElseChanged = somethingElseChanged || col.cachePolicyLocalParts() != parts;
      col.setCachePolicyLocalParts( parts );
    }
  }

  if ( changed && ( inheritChanged || ( !col.cachePolicyInherit() && somethingElseChanged ) ) ) {
    *changed = true;
  }

  return end;
}

QByteArray HandlerHelper::cachePolicyToByteArray( const Collection &col )
{
  QByteArray rv = AKONADI_PARAM_CACHEPOLICY " (";
  rv += AKONADI_PARAM_INHERIT " " + ( col.cachePolicyInherit() ? QByteArray( "true" ) : QByteArray( "false" ) );
  rv += " " AKONADI_PARAM_INTERVAL " " + QByteArray::number( col.cachePolicyCheckInterval() );
  rv += " " AKONADI_PARAM_CACHETIMEOUT " " + QByteArray::number( col.cachePolicyCacheTimeout() );
  rv += " " AKONADI_PARAM_SYNCONDEMAND " " + ( col.cachePolicySyncOnDemand() ? QByteArray( "true" ) : QByteArray( "false" ) );
  rv += " " AKONADI_PARAM_LOCALPARTS " (" + col.cachePolicyLocalParts().toLatin1() + ')';
  rv += ')';
  return rv;
}

QByteArray HandlerHelper::collectionToByteArray( const Collection &col, bool hidden, bool includeStatistics,
                                                 int ancestorDepth, const QStack<Collection> &ancestors )
{
  QByteArray b = QByteArray::number( col.id() ) + ' '
               + QByteArray::number( col.parentId() ) + " (";

  b += AKONADI_PARAM_NAME " " + ImapParser::quote( col.name().toUtf8() ) + ' ';
  if ( hidden ) {
    b += AKONADI_PARAM_MIMETYPE " () ";
  } else {
    b += AKONADI_PARAM_MIMETYPE " (" + MimeType::joinByName( col.mimeTypes(), QLatin1String( " " ) ).toLatin1() + ") ";
  }
  b += AKONADI_PARAM_REMOTEID " " + ImapParser::quote( col.remoteId().toUtf8() );
  b += " " AKONADI_PARAM_REMOTEREVISION " " + ImapParser::quote( col.remoteRevision().toUtf8() );
  b += " " AKONADI_PARAM_RESOURCE " " + ImapParser::quote( col.resource().name().toUtf8() );
  b += " " AKONADI_PARAM_VIRTUAL " " + QByteArray::number( col.isVirtual() ) + ' ';

  if ( includeStatistics ) {
    qint64 itemCount, itemSize;
    if ( itemStatistics( col, itemCount, itemSize ) ) {
      b += AKONADI_ATTRIBUTE_MESSAGES " " + QByteArray::number( itemCount ) + ' ';
      // itemWithFlagCount is twice as fast as itemWithoutFlagCount, so emulated that...
      b += AKONADI_ATTRIBUTE_UNSEEN " ";
      b += QByteArray::number( itemCount - itemWithFlagsCount( col, QStringList() << QLatin1String( AKONADI_FLAG_SEEN )
                                                                                  << QLatin1String( AKONADI_FLAG_IGNORED ) ) );
      b += " " AKONADI_PARAM_SIZE " " + QByteArray::number( itemSize ) + ' ';
    }
  }

  if ( !col.queryLanguage().isEmpty() ) {
    b += AKONADI_PARAM_PERSISTENTSEARCH " ";
    QList<QByteArray> args;
    args.append( AKONADI_PARAM_PERSISTENTSEARCH_QUERYLANG );
    args.append( col.queryLanguage().toLatin1() );
    args.append( AKONADI_PARAM_PERSISTENTSEARCH_QUERYSTRING );
    args.append( ImapParser::quote( col.queryString().toUtf8() ) );
    b += ImapParser::quote( "(" + ImapParser::join( args, " " ) + ")" );
    b += ' ';
  }

  b += HandlerHelper::cachePolicyToByteArray( col ) + ' ';
  if ( ancestorDepth > 0 ) {
    b += HandlerHelper::ancestorsToByteArray( ancestorDepth, ancestors ) + ' ';
  }

  const CollectionAttribute::List attrs = col.attributes();
  for ( int i = 0; i < attrs.size(); ++i ) {
    const CollectionAttribute &attr = attrs[i];
    b += attr.type() + ' ' + ImapParser::quote( attr.value() );
    if ( i != attrs.size() - 1 ) {
      b += ' ';
    }
  }
  b+= ')';

  return b;
}

QByteArray HandlerHelper::ancestorsToByteArray( int ancestorDepth, const QStack<Collection> &_ancestors )
{
  QByteArray b;
  if ( ancestorDepth > 0 ) {
    b += AKONADI_PARAM_ANCESTORS " (";
    QStack<Collection> ancestors( _ancestors );
    for ( int i = 0; i < ancestorDepth; ++i ) {
      if ( ancestors.isEmpty() ) {
        b += "(0 \"\")";
        break;
      }
      b += '(';
      const Collection c = ancestors.pop();
      b += QByteArray::number( c.id() ) + " ";
      b += ImapParser::quote( c.remoteId().toUtf8() );
      b += ")";
      if ( i != ancestorDepth - 1 ) {
        b += ' ';
      }
    }
    b += ')';
  }
  return b;
}

int HandlerHelper::parseDepth( const QByteArray &depth )
{
  if ( depth.isEmpty() ) {
    throw ImapParserException( "No depth specified" );
  }
  if ( depth == "INF" ) {
    return INT_MAX;
  }
  bool ok = false;
  int result = depth.toInt( &ok );
  if ( !ok ) {
    throw ImapParserException( "Invalid depth argument" );
  }
  return result;
}

Akonadi::Flag::List Akonadi::HandlerHelper::resolveFlags( const QList< QByteArray > &flagNames )
{
  Flag::List flagList;
  Q_FOREACH ( const QByteArray &flagName, flagNames ) {
    Flag flag = Flag::retrieveByName( QString::fromUtf8( flagName ) );
    if ( !flag.isValid() ) {
      flag = Flag( QString::fromUtf8( flagName ) );
      if ( !flag.insert() ) {
        throw HandlerException( "Unable to create flag" );
      }
    }
    flagList.append( flag );
  }
  return flagList;
}
