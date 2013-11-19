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
#include "search/searchmanager.h"
#include "storage/selectquerybuilder.h"

#include <libs/protocol_p.h>

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
  QList<QByteArray> mimeTypes;
  QVector<qint64> collectionIds;
  bool recursive = false;
  QByteArray queryString;

  // Backward compatibility
  if ( !connection()->capabilities().serverSideSearch() ) {
    queryString = m_streamParser->readString();
  } else {
    while (m_streamParser->hasString()) {
      const QByteArray param = m_streamParser->readString();
      if ( param == AKONADI_PARAM_MIMETYPE ) {
        mimeTypes = m_streamParser->readParenthesizedList();
      } else if ( param == AKONADI_PARAM_COLLECTIONS ) {
        QList<QByteArray> list = m_streamParser->readParenthesizedList();
        Q_FOREACH ( const QByteArray &col, list ) {
          collectionIds << col.toLongLong();
        }
      } else if ( param == AKONADI_PARAM_RECURSIVE ) {
        recursive = true;
      } else if ( param == AKONADI_PARAM_QUERY ) {
        queryString = m_streamParser->readString();
      } else {
        return failureResponse( "Invalid parameter" );
      }
    }
  }

  if ( queryString.isEmpty() ) {
    return failureResponse( "No query specified" );
  }

  Collection::List collections;
  if ( recursive ) {
    Q_FOREACH ( qint64 collId, collections ) {
      collections << listCollectionsRecursive( collections );
    }
  }

  QSet<qint64> uids = SearchManager::search( QString::fromUtf8( queryString ), collections );
  if ( uids.isEmpty() ) {
    m_streamParser->readUntilCommandEnd(); // skip the fetch scope
    return successResponse( "SEARCH completed" );
  }

  // create imap query
  ImapSet itemSet;
  itemSet.add( uids.toList() );
  Scope scope( Scope::Uid );
  scope.setUidSet( itemSet );

  FetchHelper fetchHelper( connection(), scope );
  fetchHelper.setStreamParser( m_streamParser );
  connect( &fetchHelper, SIGNAL(responseAvailable(Akonadi::Response)),
           this, SIGNAL(responseAvailable(Akonadi::Response)) );

  if ( !fetchHelper.parseStream( AKONADI_CMD_SEARCH ) ) {
    return false;
  }

  successResponse( "SEARCH completed" );
  return true;
}

QVector<qint64> Search::listCollectionsRecursive( const QVector<qint64> &ancestors )
{
  QVector<qint64> recursiveChildren;
  Q_FOREACH ( qint64 ancestor, ancestors ) {
    SelectQueryBuilder<Collection> qb;
    qb.addColumn( Collection::idFullColumnName() );
    qb.addValueCondition( Collection::parentIdFullColumnName() );
    qb.exec();
    const QVector<qint64> children = qb.result();
    if ( !children.isEmpty() ) {
      recursiveChildren << children;
      recursiveChildren << listCollectionsRecursive( children );
    }
  }

  return recursiveChildren;
}
