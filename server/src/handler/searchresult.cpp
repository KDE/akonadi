/*
 * Copyright 2013  Daniel Vrátil <dvratil@redhat.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "searchresult.h"
#include "imapstreamparser.h"
#include "protocol_p.h"
#include "storage/selectquerybuilder.h"
#include "storage/itemqueryhelper.h"
#include "search/searchmanager.h"

using namespace Akonadi;

SearchResult::SearchResult( const Scope &scope )
  : Handler()
  , mScope( scope )
{
}

SearchResult::~SearchResult()
{
}


bool SearchResult::parseStream()
{
  if ( mScope.scope() != Scope::Uid && mScope.scope() != Scope::Rid ) {
    throw HandlerException( "Only UID or RID scopes are allowed in SEARECH_RESULT" );
  }

  const QByteArray searchId = m_streamParser->readString();

  // sequence set
  mScope.parseScope( m_streamParser );

  QSet<qint64> ids;
  if ( mScope.scope() == Scope::Rid ) {
    SelectQueryBuilder<PimItem> qb;
    qb.addColumn( PimItem::idFullColumnName() );
    ItemQueryHelper::remoteIdToQuery( mScope.ridSet(), connection(), qb );

    if ( !qb.exec() ) {
      throw HandlerException( "Failed to convert RIDs to UIDs" );
    }

    QSqlQuery query = qb.query();
    while ( query.isValid() ) {
      ids << query.value( 0 ).toLongLong();
      query.next();
    }
  } else if ( mScope.scope() == Scope::Uid ) {
    Q_FOREACH ( const ImapInterval &interval, mScope.uidSet().intervals() ) {
      if ( !interval.hasDefinedBegin() && !interval.hasDefinedEnd() ) {
        throw HandlerException( "Open UID intervals not allowed in SEARCH_RESULT" );
      }

      for ( int i = interval.begin(); i <= interval.end(); ++i ) {
        ids << i;
      }
    }
  }

  SearchManager::instance()->searchResultsAvailable( searchId, ids, connection() );

  return true;
}
