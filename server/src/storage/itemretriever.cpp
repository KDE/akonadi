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

AkonadiConnection *ItemRetriever::connection() const
{
  return mConnection;
}

void ItemRetriever::setRetrieveParts(const QStringList& parts)
{
  mParts = parts;
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
    mParts += QLatin1String( "PLD:RFC822" );
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

Akonadi::QueryBuilder ItemRetriever::buildGenericItemQuery() const
{
  // make sure the columns indexes here and in the constants defined in the header match
  QueryBuilder itemQuery;
  itemQuery.addTable( PimItem::tableName() );
  itemQuery.addTable( MimeType::tableName() );
  itemQuery.addTable( Collection::tableName() );
  itemQuery.addTable( Resource::tableName() );

  itemQuery.addColumn( PimItem::idFullColumnName() );
  itemQuery.addColumn( PimItem::remoteIdFullColumnName() );
  itemQuery.addColumn( MimeType::nameFullColumnName() );
  itemQuery.addColumn( Resource::nameFullColumnName() );

  itemQuery.addColumnCondition( PimItem::mimeTypeIdFullColumnName(), Query::Equals, MimeType::idFullColumnName() );
  itemQuery.addColumnCondition( PimItem::collectionIdFullColumnName(), Query::Equals, Collection::idFullColumnName() );
  itemQuery.addColumnCondition( Collection::resourceIdFullColumnName(), Query::Equals, Resource::idFullColumnName() );

  if ( mScope.scope() != Scope::Invalid )
    ItemQueryHelper::scopeToQuery( mScope, mConnection, itemQuery );
  else
    ItemQueryHelper::itemSetToQuery( mItemSet, itemQuery, mCollection );

  itemQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

  return itemQuery;
}


Akonadi::QueryBuilder ItemRetriever::buildItemQuery() const
{
  QueryBuilder itemQuery = buildGenericItemQuery();

  // prevent a resource to trigger item retrieval from itself
  if ( mConnection ) {
    itemQuery.addValueCondition( Resource::nameFullColumnName(), Query::NotEquals,
                                 QString::fromLatin1( mConnection->sessionId() ) );
  }

  if ( !itemQuery.exec() )
    throw ItemRetrieverException( "Unable to list items" );

  itemQuery.query().next();
  return itemQuery;
}

Akonadi::QueryBuilder ItemRetriever::buildGenericPartQuery() const
{
  QueryBuilder partQuery;
  partQuery.addTable( PimItem::tableName() );
  partQuery.addTable( Part::tableName() );
  partQuery.addColumn( PimItem::idFullColumnName() );
  partQuery.addColumn( Part::nameFullColumnName() );
  partQuery.addColumn( Part::dataFullColumnName() );
  partQuery.addColumn( Part::externalFullColumnName() );
  partQuery.addColumnCondition( PimItem::idFullColumnName(), Query::Equals, Part::pimItemIdFullColumnName() );
  partQuery.addSortColumn( PimItem::idFullColumnName(), Query::Ascending );

  return partQuery;
}

Akonadi::QueryBuilder ItemRetriever::buildPartQuery() const
{
  QueryBuilder partQuery = buildGenericPartQuery();
  partQuery.addValueCondition( QString::fromLatin1( "substr(%1, 1, 4 )" ).arg( Part::nameFullColumnName() ), Query::Equals, QLatin1String( "PLD:" ) );

  if ( !mParts.isEmpty() )
    partQuery.addValueCondition( Part::nameFullColumnName(), Query::In, mParts );

  ItemQueryHelper::itemSetToQuery( mItemSet, partQuery, mCollection );
  return partQuery;
}

void ItemRetriever::exec()
{
  qDebug() << "ItemRetriever::exec()";
  if ( mParts.isEmpty() && !mFullPayload )
    return;

  // TODO: I'm sure this can be done with a single query instead of manually
  QueryBuilder itemQuery = buildItemQuery();
  QueryBuilder partQuery = buildPartQuery();
  if ( !partQuery.exec() )
    throw ItemRetrieverException( "Unable to retrieve item parts" );
  partQuery.query().next();

  QList<ItemRetrievalRequest*> requests;
  while ( itemQuery.query().isValid() ) {
    const qint64 pimItemId = itemQuery.query().value( sItemQueryPimItemIdColumn ).toLongLong();
    QStringList missingParts = mParts;
    while ( partQuery.query().isValid() ) {
      const qint64 id = partQuery.query().value( sPartQueryPimIdColumn ).toLongLong();
      if ( id < pimItemId ) {
        partQuery.query().next();
        continue;
      } else if ( id > pimItemId ) {
        break;
      }
      const QString partName = partQuery.query().value( sPartQueryNameColumn ).toString();
      QByteArray data = partQuery.query().value( sPartQueryDataColumn ).toByteArray();
      // FIXME: loading the actual data is not needed here!
      // ### maybe add an flag indicating if a part is cached?
      data = PartHelper::translateData( data, partQuery.query().value( sPartQueryExternalColumn ).toBool() );
      if ( data.isNull() ) {
        if ( mFullPayload && !missingParts.contains( partName ) )
          missingParts << partName;
      } else {
        missingParts.removeAll( partName );
      }
      partQuery.query().next();
    }
    if ( !missingParts.isEmpty() ) {
      QStringList missingPayloadIds;
      foreach ( const QString &s, missingParts )
        missingPayloadIds << s.mid( 4 );

      ItemRetrievalRequest *req = new ItemRetrievalRequest();
      req->id = pimItemId;
      req->remoteId = itemQuery.query().value( sItemQueryPimItemRidColumn ).toString().toUtf8();
      req->mimeType = itemQuery.query().value( sItemQueryMimeTypeColumn ).toString().toUtf8();
      req->resourceId = itemQuery.query().value( sItemQueryResouceColumn ).toString();
      req->parts = missingPayloadIds;

      // TODO: how should we handle retrieval errors here? so far they have been ignored,
      // which makes sense in some cases, do we need a command parameter for this?
      if ( driverName().startsWith( QLatin1String( "QSQLITE" ) ) )
        requests.append( req );
      else {
        try {
          ItemRetrievalManager::instance()->requestItemDelivery( req );
        } catch ( const ItemRetrieverException &e ) {
          akError() << e.type() << ": " << e.what();
        }
      }
    }
    itemQuery.query().next();
  }

  if ( driverName().startsWith( QLatin1String( "QSQLITE" ) ) ) {
    akDebug() << "Closing queries and sending out requests.";

    // Close the queries so we can start writing without endig up in a lock.
    partQuery.query().finish();
    itemQuery.query().finish();

    // Now both reading queries are closed, start sending out the
    foreach ( ItemRetrievalRequest *req, requests ) {
      // TODO: how should we handle retrieval errors here? so far they have been ignored,
      // which makes sense in some cases, do we need a command parameter for this?
      try {
        ItemRetrievalManager::instance()->requestItemDelivery( req );
      } catch ( const ItemRetrieverException &e ) {
        akError() << e.type() << ": " << e.what();
      }
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
  qDebug() << "ItemRetriever::exec() done";
}

QString ItemRetriever::driverName()
{
  return mConnection->storageBackend()->database().driverName();
}
