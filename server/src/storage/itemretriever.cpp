/*
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>
    Copyright (c) 2010 Milian Wolff <mail@milianw.de>

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

#include "itemretriever.h"

#include "akdebug.h"
#include "akonadiconnection.h"
#include "storage/datastore.h"
#include "storage/itemqueryhelper.h"
#include "storage/itemretrievalmanager.h"
#include "storage/itemretrievalrequest.h"
#include "storage/parthelper.h"
#include "storage/querybuilder.h"

#include <QDebug>

using namespace Akonadi;

ItemRetriever::ItemRetriever( AkonadiConnection *connection ) :
  mScope( Scope::Invalid ),
  mConnection( connection ),
  mFullPayload( false ),
  mRecursive( false )
{ }

ItemRetriever::~ItemRetriever()
{ }

AkonadiConnection *ItemRetriever::connection() const
{
  return mConnection;
}

void ItemRetriever::setRetrieveParts(const QStringList& parts)
{
  mParts = parts;
  // HACK, we need a full payload available flag in PimItem
  if ( mFullPayload && !mParts.contains( QLatin1String( "PLD:RFC822" ) ) )
    mParts.append( QLatin1String( "PLD:RFC822" ) );
}

void ItemRetriever::setItemSet(const ImapSet& set, const Collection &collection )
{
  mItemSet = set;
  mCollection = collection;
}

void ItemRetriever::setItemSet(const ImapSet& set, bool isUid)
{
  Q_ASSERT( mConnection );
  if ( !isUid && mConnection->selectedCollectionId() >= 0 )
    setItemSet( set, mConnection->selectedCollection() );
  else
    setItemSet( set );
}

void ItemRetriever::setItem( const Akonadi::Entity::Id& id )
{
  ImapSet set;
  set.add( ImapInterval( id, id ) );
  mItemSet = set;
  mCollection = Collection();
}

void ItemRetriever::setRetrieveFullPayload(bool fullPayload)
{
  mFullPayload = fullPayload;
  // HACK, we need a full payload available flag in PimItem
  if ( fullPayload && !mParts.contains( QLatin1String( "PLD:RFC822" ) ) )
    mParts.append( QLatin1String( "PLD:RFC822" ) );
}

void ItemRetriever::setCollection(const Collection& collection, bool recursive)
{
  mCollection = collection;
  mItemSet = ImapSet();
  mRecursive = recursive;
}

void ItemRetriever::setScope(const Scope& scope)
{
  mScope = scope;
}

Scope ItemRetriever::scope() const
{
  return mScope;
}

QStringList ItemRetriever::retrieveParts() const
{
  return mParts;
}

enum QueryColumns {
  PimItemIdColumn,
  PimItemRidColumn,

  MimeTypeColumn,

  ResourceColumn,

  PartNameColumn,
  PartDataColumn,
  PartExternalColumn
};

QSqlQuery ItemRetriever::buildQuery() const
{
  QueryBuilder qb( PimItem::tableName() );

  qb.addJoin( QueryBuilder::InnerJoin, MimeType::tableName(), PimItem::mimeTypeIdFullColumnName(), MimeType::idFullColumnName() );

  qb.addJoin( QueryBuilder::InnerJoin, Collection::tableName(), PimItem::collectionIdFullColumnName(), Collection::idFullColumnName() );

  qb.addJoin( QueryBuilder::InnerJoin, Resource::tableName(), Collection::resourceIdFullColumnName(), Resource::idFullColumnName() );

  Query::Condition partJoinCondition;
  partJoinCondition.addColumnCondition(PimItem::idFullColumnName(), Query::Equals, Part::pimItemIdFullColumnName());
  if ( !mFullPayload && !mParts.isEmpty() ) {
    partJoinCondition.addValueCondition( Part::nameFullColumnName(), Query::In, mParts );
  }
  partJoinCondition.addValueCondition( QString::fromLatin1( "substr(%1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "PLD:" ) );
  qb.addJoin( QueryBuilder::LeftJoin, Part::tableName(), partJoinCondition );

  qb.addColumn( PimItem::idFullColumnName() );
  qb.addColumn( PimItem::remoteIdFullColumnName() );
  qb.addColumn( MimeType::nameFullColumnName() );
  qb.addColumn( Resource::nameFullColumnName() );
  qb.addColumn( Part::nameFullColumnName() );
  qb.addColumn( Part::dataFullColumnName() );
  qb.addColumn( Part::externalFullColumnName() );

  if ( mScope.scope() != Scope::Invalid )
    ItemQueryHelper::scopeToQuery( mScope, mConnection, qb );
  else
    ItemQueryHelper::itemSetToQuery( mItemSet, qb, mCollection );

  // prevent a resource to trigger item retrieval from itself
  if ( mConnection ) {
    qb.addValueCondition( Resource::nameFullColumnName(), Query::NotEquals,
                          QString::fromLatin1( mConnection->sessionId() ) );
  }

  qb.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

  if ( !qb.exec() )
    throw ItemRetrieverException( "Unable to retrieve items" );

  qb.query().next();

  return qb.query();
}


void ItemRetriever::exec()
{
  if ( mParts.isEmpty() && !mFullPayload )
    return;

  QSqlQuery query = buildQuery();
  ItemRetrievalRequest* lastRequest = 0;
  QList<ItemRetrievalRequest*> requests;

  QStringList parts;
  foreach ( const QString part, mParts ) {
    if ( part.startsWith( QLatin1String( "PLD:" ) ) ) {
      parts << part.mid(4);
    }
  }

  while ( query.isValid() ) {
    const qint64 pimItemId = query.value( PimItemIdColumn ).toLongLong();
    if ( !lastRequest || lastRequest->id != pimItemId ) {
      lastRequest = new ItemRetrievalRequest();
      lastRequest->id = pimItemId;
      lastRequest->remoteId = query.value( PimItemRidColumn ).toString().toUtf8();
      lastRequest->mimeType = query.value( MimeTypeColumn ).toString().toUtf8();
      lastRequest->resourceId = query.value( ResourceColumn ).toString();
      lastRequest->parts = parts;
      requests << lastRequest;
    }

    if ( query.value( PartNameColumn ).isNull() ) {
      // LEFT JOIN did not find anything, retrieve all parts
      query.next();
      continue;
    }

    QByteArray data = query.value( PartDataColumn ).toByteArray();
    data = PartHelper::translateData( data, query.value( PartExternalColumn ).toBool() );
    QString partName = query.value( PartNameColumn ).toString();
    Q_ASSERT( partName.startsWith( QLatin1String( "PLD:" ) ) );
    partName = partName.mid( 4 );
    if ( data.isNull() ) {
      // request update for this part
      if ( mFullPayload && !lastRequest->parts.contains( partName ) )
        lastRequest->parts << partName;
    } else {
      // data available, don't request update
      lastRequest->parts.removeAll( partName );
      if ( lastRequest->parts.isEmpty() ) {
        delete requests.takeLast();
        lastRequest = 0;
      }
    }
    query.next();
  }

  akDebug() << "Closing queries and sending out requests.";

  query.finish();

  foreach ( ItemRetrievalRequest* request, requests ) {
    Q_ASSERT( !request->parts.isEmpty() );
    // TODO: how should we handle retrieval errors here? so far they have been ignored,
    // which makes sense in some cases, do we need a command parameter for this?
    try {
      ItemRetrievalManager::instance()->requestItemDelivery( request );
    } catch ( const ItemRetrieverException &e ) {
      akError() << e.type() << ": " << e.what();
    }
  }

  // retrieve items in child collections if requested
  if ( mRecursive && mCollection.isValid() ) {
    foreach ( const Collection &col, mCollection.children() ) {
      ItemRetriever retriever( mConnection );
      retriever.setCollection( col, mRecursive );
      retriever.setRetrieveParts( mParts );
      retriever.setRetrieveFullPayload( mFullPayload );
      retriever.exec();
    }
  }
}

QString ItemRetriever::driverName()
{
  return mConnection->storageBackend()->database().driverName();
}
