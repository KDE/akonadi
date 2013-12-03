/*
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#include "searchresultsretriever.h"
#include "akonadiconnection.h"
#include "searchmanager.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionqueryhelper.h"
#include "akdebug.h"

#include <QSqlError>

Akonadi::SearchResultsRetriever::SearchResultsRetriever( Akonadi::AkonadiConnection *connection )
  : mConnection( connection )
{
}

Akonadi::SearchResultsRetriever::~SearchResultsRetriever()
{
}

void Akonadi::SearchResultsRetriever::setQuery( const QString &query )
{
  mQuery = query;
}

void Akonadi::SearchResultsRetriever::setCollections( const QVector<qint64> &collectionsIds )
{
  mCollections = collectionsIds;
}

void Akonadi::SearchResultsRetriever::setMimeTypes( const QStringList &mimeTypes )
{
  mMimeTypes = mimeTypes;
}

QSet<qint64> Akonadi::SearchResultsRetriever::exec( bool *ok )
{
  if ( ok ) {
    *ok = false;
  }

  QueryBuilder qb( Collection::tableName() );
  qb.addJoin( QueryBuilder::InnerJoin, Resource::tableName(),
              Collection::resourceIdFullColumnName(),
              Resource::idFullColumnName());
  qb.addColumn( Collection::idFullColumnName() );
  qb.addColumn( Resource::nameFullColumnName() );

  QVariantList list;
  Q_FOREACH ( qint64 collection, mCollections ) {
    list << collection;
  }
  qb.addValueCondition( Collection::idFullColumnName(), Query::In, list );

  if ( !qb.exec() ) {
    throw HandlerException( qb.query().lastError().text() );
  }

  QSqlQuery query = qb.query();
  if ( !query.next() ) {
    return QSet<qint64>();
  }

  QList<SearchRequest*> requests;
  while ( query.isValid() ) {
    SearchRequest *request = new SearchRequest;
    request->id = mConnection->sessionId();
    request->query = mQuery;
    request->mimeTypes = mMimeTypes;
    request->collectionId = query.value( 0 ).toLongLong();
    request->resourceId = query.value( 1 ).toString();
    requests << request;

    query.next();
  }

  query.finish();

  QSet<qint64> result;
  Q_FOREACH ( SearchRequest *request, requests ) {
    try {
      akDebug() << "Searching in" << request->resourceId << "...";
      result.unite( SearchManager::instance()->search( request ) );
    } catch ( const SearchResultsRetrieverException &e ) {
      akError() << e.type() << ":" << e.what();
      return QSet<qint64>();
    }
  }

  if ( ok ) {
    *ok = true;
  }
  return result;
}
