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
#include "config-akonadi.h"
#include "connection.h"
#include "fetchhelper.h"
#include "handlerhelper.h"
#include "searchhelper.h"
#include "imapstreamparser.h"
#include "nepomuksearch.h"
#include "response.h"
#include "search/searchrequest.h"
#include "search/searchmanager.h"

#include "libs/protocol_p.h"

#include <QtCore/QStringList>

using namespace Akonadi::Server;

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
  bool recursive = false, remote = false;
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
      } else if ( param == AKONADI_PARAM_REMOTE ) {
        remote = true;
      } else if ( param == AKONADI_PARAM_QUERY ) {
        queryString = m_streamParser->readUtf8String();
        // TODO: This is an ugly hack, but we assume QUERY is the last parameter,
        // followed only by fetch scope, which we parse separately below
        break;
      } else {
        return failureResponse( "Invalid parameter" );
      }
    }

    if ( queryString.isEmpty() ) {
      return failureResponse( "No query specified" );
    }

    QVector<qint64> collections;
    if ( collectionIds.isEmpty() ) {
      collectionIds << 0;
      recursive = true;
    }

    if ( recursive ) {
      collections << SearchHelper::matchSubcollectionsByMimeType( collectionIds, mimeTypes );
    } else {
      collections = collectionIds;
    }

    akDebug() << "SEARCH:";
    akDebug() << "\tQuery:" << queryString;
    akDebug() << "\tMimeTypes:" << mimeTypes;
    akDebug() << "\tCollections:" << collections;
    akDebug() << "\tRemote:" << remote;
    akDebug() << "\tRecursive" << recursive;

    if ( collections.isEmpty() ) {
      m_streamParser->readUntilCommandEnd();
      return successResponse( "Search done" );
    }

    // Read the fetch scope
    mFetchScope = FetchScope( m_streamParser );
    // Read any newlines
    m_streamParser->readUntilCommandEnd();

    SearchRequest request( connection()->sessionId() );
    request.setCollections( collections );
    request.setMimeTypes( mimeTypes );
    request.setQuery( queryString );
    request.setRemoteSearch( remote );
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

  QStringList uids;
#ifdef HAVE_SOPRANO
  NepomukSearch *service = new NepomukSearch;
  uids = service->search( queryString );
  delete service;
#else
  akError() << "Akonadi has been built without Nepomuk support!";
  return;
#endif

  if ( uids.isEmpty() ) {
    return;
  }

  QSet<qint64> results;
  Q_FOREACH ( const QString &uid, uids ) {
    results.insert( uid.toLongLong() );
  }

  slotResultsAvailable( results );
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
  connect( &fetchHelper, SIGNAL(responseAvailable(Akonadi::Server::Response)),
          this, SIGNAL(responseAvailable(Akonadi::Server::Response)) );

  fetchHelper.fetchItems( AKONADI_CMD_SEARCH );
}
