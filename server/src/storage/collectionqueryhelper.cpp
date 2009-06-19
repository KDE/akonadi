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

#include "collectionqueryhelper.h"

#include "akonadiconnection.h"
#include "entities.h"
#include "storage/querybuilder.h"
#include "libs/imapset_p.h"
#include "handler/scope.h"
#include "handler.h"
#include "queryhelper.h"

using namespace Akonadi;

void CollectionQueryHelper::remoteIdToQuery(const QStringList& rids, AkonadiConnection* connection, QueryBuilder& qb)
{
  if ( rids.size() == 1 )
    qb.addValueCondition( Collection::remoteIdFullColumnName(), Query::Equals, rids.first() );
  else
    qb.addValueCondition( Collection::remoteIdFullColumnName(), Query::In, rids );

  if ( connection->resourceContext().isValid() ) {
    qb.addValueCondition( Collection::resourceIdFullColumnName(), Query::Equals, connection->resourceContext().id() );
  }
}

void CollectionQueryHelper::scopeToQuery(const Scope& scope, AkonadiConnection* connection, QueryBuilder& qb)
{
  if ( scope.scope() == Scope::None || scope.scope() == Scope::Uid ) {
    QueryHelper::setToQuery( scope.uidSet(), Collection::idFullColumnName(), qb );
  } else if ( scope.scope() == Scope::Rid ) {
    if ( connection->selectedCollectionId() <= 0 && !connection->resourceContext().isValid() )
      throw HandlerException( "Operations based on remote identifiers require a resource or collection context" );
    CollectionQueryHelper::remoteIdToQuery( scope.ridSet(), connection, qb );
  } else
    throw HandlerException( "WTF?" );
}
