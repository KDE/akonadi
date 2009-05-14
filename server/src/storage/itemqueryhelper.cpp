/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include "itemqueryhelper.h"

#include "akonadiconnection.h"
#include "entities.h"
#include "storage/querybuilder.h"
#include "libs/imapset_p.h"

using namespace Akonadi;

void ItemQueryHelper::itemSetToQuery(const ImapSet& set, QueryBuilder& qb, const Collection& collection )
{
  Query::Condition cond( Query::Or );
  foreach ( const ImapInterval i, set.intervals() ) {
    if ( i.hasDefinedBegin() && i.hasDefinedEnd() ) {
      if ( i.size() == 1 ) {
        cond.addValueCondition( PimItem::idFullColumnName(), Query::Equals, i.begin() );
      } else {
        Query::Condition subCond( Query::And );
        subCond.addValueCondition( PimItem::idFullColumnName(), Query::GreaterOrEqual, i.begin() );
        subCond.addValueCondition( PimItem::idFullColumnName(), Query::LessOrEqual, i.end() );
        cond.addCondition( subCond );
      }
    } else if ( i.hasDefinedBegin() ) {
      cond.addValueCondition( PimItem::idFullColumnName(), Query::GreaterOrEqual, i.begin() );
    } else if ( i.hasDefinedEnd() ) {
      cond.addValueCondition( PimItem::idFullColumnName(), Query::LessOrEqual, i.end() );
    }
  }
  if ( !cond.isEmpty() )
    qb.addCondition( cond );

  if ( collection.isValid() ) {
    // FIXME: we probably want to do both paths here in all cases, but that is apparently
    // non-trivial with SQL
    if ( collection.resource().name() == QLatin1String("akonadi_search_resource") ||
      collection.resource().name().contains( QLatin1String("nepomuk") ) )
    {
      qb.addTable( CollectionPimItemRelation::tableName() );
      qb.addValueCondition( CollectionPimItemRelation::leftFullColumnName(), Query::Equals, collection.id() );
      qb.addColumnCondition( CollectionPimItemRelation::rightFullColumnName(), Query::Equals,
                             PimItem::idFullColumnName() );
    } else {
      qb.addValueCondition( PimItem::collectionIdColumn(), Query::Equals, collection.id() );
    }
  }
}

void ItemQueryHelper::itemSetToQuery(const ImapSet& set, bool isUid, AkonadiConnection* connection, QueryBuilder& qb)
{
  if ( !isUid && connection->selectedCollectionId() >= 0 )
    itemSetToQuery( set, qb, connection->selectedCollection() );
  else
    itemSetToQuery( set, qb );
}

void ItemQueryHelper::remoteIdToQuery(const QStringList& rids, AkonadiConnection* connection, QueryBuilder& qb)
{
  if ( rids.size() == 1 ) 
    qb.addValueCondition( PimItem::remoteIdFullColumnName(), Query::Equals, rids.first() );
  else
    qb.addValueCondition( PimItem::remoteIdFullColumnName(), Query::In, rids );

  if ( connection->selectedCollectionId() > 0 ) {
    qb.addTable( Collection::tableName() );
    qb.addValueCondition( PimItem::collectionIdFullColumnName(), Query::Equals, connection->selectedCollectionId() );
  } else if ( connection->resourceContext().isValid() ) {
    qb.addTable( Collection::tableName() );
    qb.addColumnCondition( PimItem::collectionIdFullColumnName(), Query::Equals, Collection::idFullColumnName() );
    qb.addValueCondition( Collection::resourceIdFullColumnName(), Query::Equals, connection->resourceContext().id() );
  }
}
