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
#include "searchcollector.h"
#include "storage/selectquerybuilder.h"
#include "storage/collectionqueryhelper.h"
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

QVector<qint64> Akonadi::SearchResultsRetriever::exec( bool *ok )
{
  SelectQueryBuilder<Collection> qb;
  qb.addJoin( QueryBuilder::InnerJoin, ResourceTable::name(),
              CollectionTable::resourceIdFullColumnName(),
              ResourceTable::idFullColumnName());
  qb.addColumn( CollectionTable::idFullColumnName() );
  qb.addColumn( ResourceTable::nameFullColumnName() );
  CollectionQueryHelper::scopeToQuery( mCollections, mConnection, qb );

  if ( !qb.exec() ) {
    throw HandlerException( qb.query().lastError().text() );
  }

  QSqlQuery query = qb.query();
  if ( !query.next() ) {
    return QVector<qint64>();
  }

  QMultiMap<QByteArray, qint64> map;
  do {
    map.insert( query.value( 0 ).toByteArray(), query.value( 1 ).toLongLong() );
  } while ( query.next() );

  SearchTask task;
  task.id = mConnection->sessionId();
  task.query = mQuery;
  task.collections = map;
  task.mimeTypes = mMimeTypes;

  return Akonadi::SearchCollector::self()->exec( &task, ok );
}
