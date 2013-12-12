/***************************************************************************
 *   Copyright (C) 2009 by Tobias Koenig <tokoe@kde.org>                   *
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

#include "search.h"

#include "akonadi.h"
#include "akonadiconnection.h"
#include "fetchhelper.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"
#include "nepomuksearch.h"
#include "response.h"
#include "storage/selectquerybuilder.h"
#include "search/agentsearchrequest.h"
#include "search/searchmanager.h"

#include "libs/protocol_p.h"

#include <QtCore/QStringList>

using namespace Akonadi;

Search::Search()
  : Handler()
{
}

Search::~Search()
{
}

bool Search::parseStream()
{
  QStringList mimeTypes;
  QVector<qint64> collectionIds;
  bool recursive = false;
  QString queryString;

  // Backward compatibility
  if ( !connection()->capabilities().serverSideSearch() ) {
    searchNepomuk();
  } else {
    while (m_streamParser->hasString()) {
      const QByteArray param = m_streamParser->readString();
      if ( param == AKONADI_PARAM_MIMETYPE ) {
        const QList<QByteArray> mt = m_streamParser->readParenthesizedList();
        mimeTypes.reserve( mt.size() );
        Q_FOREACH ( const QByteArray &ba, mt ) {
          mimeTypes.append( QString::fromLatin1( ba ) );
        }
      } else if ( param == AKONADI_PARAM_COLLECTIONS ) {
        QList<QByteArray> list = m_streamParser->readParenthesizedList();
        Q_FOREACH ( const QByteArray &col, list ) {
          collectionIds << col.toLongLong();
        }
      } else if ( param == AKONADI_PARAM_RECURSIVE ) {
        recursive = true;
      } else if ( param == AKONADI_PARAM_QUERY ) {
        queryString = m_streamParser->readUtf8String();
      } else {
        return failureResponse( "Invalid parameter" );
      }
    }

    if ( queryString.isEmpty() ) {
      return failureResponse( "No query specified" );
    }

    QVector<qint64> collections;
    if ( recursive ) {
      Q_FOREACH ( qint64 collection, collectionIds ) {
        collections << listCollectionsRecursive( QVector<qint64>() <<  collection, mimeTypes );
      }
    } else {
      collections = collectionIds;
    }

    akDebug() << "SEARCH:";
    akDebug() << "\tQuery:" << queryString;
    akDebug() << "\tMimeTypes:" << mimeTypes;
    akDebug() << "\tCollections:" << collections;

    // Read the fetch scope
    mFetchScope = FetchScope( m_streamParser );

    AgentSearchRequest request( connection() );
    request.setCollections( collections );
    request.setMimeTypes( mimeTypes );
    request.setQuery( queryString );
    connect( &request, SIGNAL(resultsAvailable(QSet<qint64>)),
            this, SLOT(slotResultsAvailable(QSet<qint64>)) );
    request.exec();

  }

  //akDebug() << "\tResult:" << uids;
  akDebug() << "\tResult:" << mAllResults.count() << "matches";

  return successResponse( "Search done" );
}

void Search::searchNepomuk()
{
  const QString queryString = m_streamParser->readUtf8String();
  mFetchScope = FetchScope( m_streamParser );

  NepomukSearch *service = new NepomukSearch;
  const QStringList uids = service->search( queryString );
  delete service;
  if ( uids.isEmpty() ) {
    return;
  }

  QSet<qint64> results;
  Q_FOREACH ( const QString &uid, uids ) {
    results.insert( uid.toLongLong() );
  }

  slotResultsAvailable( results );
}

QVector<qint64> Search::listCollectionsRecursive( const QVector<qint64> &ancestors, const QStringList &mimeTypes )
{
  QVector<qint64> recursiveChildren;
  Q_FOREACH ( qint64 ancestor, ancestors ) {
    Query::Condition mimeTypeCondition;
    mimeTypeCondition.addColumnCondition( CollectionMimeTypeRelation::rightFullColumnName(), Query::Equals, MimeType::idFullColumnName() );
    // Exclude top-level collections and collections that cannot have items!
    mimeTypeCondition.addValueCondition( MimeType::nameFullColumnName(), Query::NotEquals, QLatin1String( "inode/directory" ) );
    if ( !mimeTypes.isEmpty() ) {
      mimeTypeCondition.addValueCondition( MimeType::nameFullColumnName(), Query::In, mimeTypes );
    }

    QueryBuilder qb( Collection::tableName() );
    qb.addColumn( Collection::idFullColumnName() );
    qb.addColumn( MimeType::nameFullColumnName() );
    qb.addJoin( QueryBuilder::InnerJoin, Resource::tableName(), Resource::idFullColumnName(), Collection::resourceIdFullColumnName() );
    qb.addJoin( QueryBuilder::LeftJoin, CollectionMimeTypeRelation::tableName(), CollectionMimeTypeRelation::leftFullColumnName(), Collection::idFullColumnName() );
    qb.addJoin( QueryBuilder::LeftJoin, MimeType::tableName(), mimeTypeCondition );
    if ( ancestor == 0 ) {
      qb.addValueCondition( Collection::parentIdFullColumnName(), Query::Is, QVariant() );
    } else {
      qb.addValueCondition( Collection::parentIdFullColumnName(), Query::Equals, ancestor );
    }
    qb.addGroupColumn( Collection::idFullColumnName() );
    qb.exec();

    QSqlQuery query = qb.query();
    QVector<qint64> searchChildren;
    while ( query.next() ) {
      const qint64 id = query.value( 0 ).toLongLong();
      searchChildren << id;
      if ( !query.value( 1 ).isNull() ) {
        recursiveChildren << id;
      }
    }
    if ( !searchChildren.isEmpty() ) {
      recursiveChildren << listCollectionsRecursive( searchChildren, mimeTypes );
    }
  }

  return recursiveChildren;
}

void Search::slotResultsAvailable( const QSet<qint64> &results )
{
  QSet<qint64> newResults = results;
  newResults.subtract( mAllResults );
  mAllResults.unite( newResults );

  if ( newResults.isEmpty() ) {
    return;
  }

  // create imap query
  ImapSet itemSet;
  itemSet.add( newResults );
  Scope scope( Scope::Uid );
  scope.setUidSet( itemSet );

  FetchHelper fetchHelper( connection(), scope, mFetchScope );
  connect( &fetchHelper, SIGNAL(responseAvailable(Akonadi::Response)),
          this, SIGNAL(responseAvailable(Akonadi::Response)) );

  fetchHelper.fetchItems( AKONADI_CMD_SEARCH );
}
