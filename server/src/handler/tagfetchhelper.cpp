/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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


#include "tagfetchhelper.h"
#include "handler.h"
#include "response.h"
#include "libs/imapparser_p.h"
#include "libs/protocol_p.h"
#include "storage/querybuilder.h"
#include "storage/queryhelper.h"
#include "entities.h"
#include "connection.h"

using namespace Akonadi;
using namespace Akonadi::Server;

TagFetchHelper::TagFetchHelper( Connection *connection, const ImapSet &set )
  : QObject()
  , mConnection( connection )
  , mSet( set )
{
}

QSqlQuery TagFetchHelper::buildAttributeQuery()
{
  QueryBuilder qb( TagAttribute::tableName() );
  qb.addColumn( TagAttribute::tagIdColumn() );
  qb.addColumn( TagAttribute::typeColumn() );
  qb.addColumn( TagAttribute::valueColumn() );
  qb.addSortColumn( TagAttribute::tagIdColumn(), Query::Descending );

  QueryHelper::setToQuery( mSet, TagAttribute::tagIdColumn(), qb );

  if ( !qb.exec() ) {
    throw HandlerException( "Unable to list tag attributes" );
  }

  qb.query().next();
  return qb.query();
}

QSqlQuery TagFetchHelper::buildTagQuery()
{
  QueryBuilder qb( Tag::tableName() );
  qb.addColumns( Tag::fullColumnNames() );

  // Expose tag's remote ID only to resources
  if ( mConnection->resourceContext().isValid() ) {
    qb.addColumn( TagRemoteIdResourceRelation::remoteIdFullColumnName() );
    qb.addJoin( QueryBuilder::LeftJoin, TagRemoteIdResourceRelation::tableName(),
                Tag::idFullColumnName(), TagRemoteIdResourceRelation::tagIdFullColumnName() );
    Query::Condition joinCondition;
    joinCondition.addColumnCondition( Resource::idFullColumnName(), Query::Equals, TagRemoteIdResourceRelation::resourceIdFullColumnName() );
    joinCondition.addValueCondition( Resource::nameFullColumnName(), Query::Equals, mConnection->resourceContext().name() );
    qb.addJoin( QueryBuilder::LeftJoin, Resource::tableName(), joinCondition );
  }

  qb.addSortColumn( Tag::idFullColumnName(), Query::Descending );
  QueryHelper::setToQuery( mSet, Tag::idFullColumnName(), qb );

  if ( !qb.exec() ) {
    throw HandlerException( "Unable to list tags" );
  }

  qb.query().next();
  return qb.query();
}


bool TagFetchHelper::fetchTags(const QByteArray& responseIdentifier)
{

  QSqlQuery tagQuery = buildTagQuery();
  QSqlQuery attributeQuery = buildAttributeQuery();

  Response response;
  response.setUntagged();
  while ( tagQuery.isValid() ) {
    const qint64 tagId = tagQuery.value( 0 ).toLongLong();
    const QByteArray gid = tagQuery.value( 1 ).toByteArray();
    const qint64 parentId = tagQuery.value( 2 ).toLongLong();
    QByteArray remoteId;
    if ( mConnection->resourceContext().isValid() ) {
      remoteId = tagQuery.value( 3 ).toByteArray();
    }

    QList<QByteArray> attributes;
    attributes << AKONADI_PARAM_UID << QByteArray::number( tagId );
    attributes << AKONADI_PARAM_GID << ImapParser::quote( gid );
    attributes << AKONADI_PARAM_PARENT << QByteArray::number( parentId );
    if ( !remoteId.isEmpty() ) {
      attributes << AKONADI_PARAM_REMOTEID << remoteId;
    }

    while ( attributeQuery.isValid() ) {
      const qint64 id = attributeQuery.value( 0 ).toLongLong();
      if ( id > tagId ) {
        attributeQuery.next();
        continue;
      } else if ( id < tagId ) {
        break;
      }

      const QByteArray attrName = attributeQuery.value( 1 ).toByteArray();
      const QByteArray attrValue = attributeQuery.value( 2 ).toByteArray();

      attributes << attrName << ImapParser::quote( attrValue );
      attributeQuery.next();
    }

    QByteArray tagReply = QByteArray::number( tagId ) + ' ' + responseIdentifier + " (";
    tagReply += ImapParser::join( attributes, " " ) + ')';
    response.setUntagged();
    response.setString( tagReply );
    Q_EMIT responseAvailable( response );

    tagQuery.next();
  }

  return true;
}

