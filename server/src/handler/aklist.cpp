/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

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

#include "aklist.h"

#include <QtCore/QDebug>

#include "storage/datastore.h"
#include "storage/entity.h"
#include "storage/selectquerybuilder.h"

#include "akonadiconnection.h"
#include "response.h"
#include "handlerhelper.h"
#include "imapstreamparser.h"

#include <libs/protocol_p.h>

using namespace Akonadi;

template <typename T>
static bool intersect( const QList<typename T::Id> &l1, const QList<T> &l2 )
{
  foreach ( const T& e2, l2 ) {
    if ( l1.contains( e2.id() ) )
      return true;
  }
  return false;
}

AkList::AkList( Scope::SelectionScope scope, bool onlySubscribed ):
    Handler(),
    mScope( scope ),
    mOnlySubscribed( onlySubscribed ),
    mIncludeStatistics( false )
{}

AkList::~AkList() {}

bool AkList::listCollection(const Collection & root, int depth )
{
  // recursive listing of child collections
  bool childrenFound = false;
  if ( depth > 0 ) {
    Collection::List children = root.children();
    foreach ( const Collection &col, children ) {
      if ( listCollection( col, depth - 1 ) )
        childrenFound = true;
    }
  }

  // filter if this node isn't needed by it's children
  bool hidden = (mResource.isValid() && root.resourceId() != mResource.id())
      || (mOnlySubscribed && !root.subscribed())
      || (!mMimeTypes.isEmpty() && !intersect( mMimeTypes, root.mimeTypes()) );

  if ( !childrenFound && hidden )
    return false;

  // write out collection details
  Collection dummy = root;
  DataStore *db = connection()->storageBackend();
  db->activeCachePolicy( dummy );
  const QByteArray b = HandlerHelper::collectionToByteArray( dummy, hidden, mIncludeStatistics );

  Response response;
  response.setUntagged();
  response.setString( b );
  emit responseAvailable( response );

  return true;
}

bool AkList::parseStream()
{
  qint64 baseCollection = -1;
  QString rid;
  if ( mScope == Scope::None || mScope == Scope::Uid ) {
    bool ok = false;
    baseCollection = m_streamParser->readNumber( &ok );
    if ( !ok )
      return failureResponse( "Invalid base collection" );
  } else if ( mScope == Scope::Rid ) {
    rid = m_streamParser->readUtf8String();
    if ( rid.isEmpty() )
      throw HandlerException( "No remote identifier specified" );
  } else
    throw HandlerException( "WTF" );

  int depth;
  const QByteArray tmp = m_streamParser->readString();
  if ( tmp.isEmpty() )
    return failureResponse( "Specify listing depth" );
  if ( tmp == "INF" )
    depth = INT_MAX;
  else
    depth = tmp.toInt();

  m_streamParser->beginList();
  while ( !m_streamParser->atListEnd() ) {
    const QByteArray filter = m_streamParser->readString();
    if ( filter == AKONADI_PARAM_RESOURCE ) {
      mResource = Resource::retrieveByName( m_streamParser->readUtf8String() );
      if ( !mResource.isValid() )
        return failureResponse( "Unknown resource" );
    } else if ( filter == AKONADI_PARAM_MIMETYPE ) {
      m_streamParser->beginList();
      while ( !m_streamParser->atListEnd() ) {
        const MimeType mt = MimeType::retrieveByName( m_streamParser->readUtf8String() );
        if ( !mt.isValid() )
          throw HandlerException( "Invalid mimetype filter" );
        mMimeTypes.append( mt.id() );
      }
    }
  }

  if ( m_streamParser->hasList() ) { // We got extra options
    QList<QByteArray> options = m_streamParser->readParenthesizedList();
    for ( int i = 0; i < options.count() - 1; i += 2 ) {
      if ( options.at( i ) == "STATISTICS" ) {
        if ( QString::fromLatin1( options.at( i + 1 ) ).compare( QLatin1String( "true" ), Qt::CaseInsensitive ) == 0 ) {
          mIncludeStatistics = true;
        }
      } else {
        return failureResponse( "Invalid option parameter" );
      }
    }
  }

  Collection::List collections;
  if ( baseCollection != 0 ) { // not root
    Collection col;
    if ( mScope == Scope::None || mScope == Scope::Uid ) {
       col = Collection::retrieveById( baseCollection );
    } else if ( mScope == Scope::Rid ) {
      SelectQueryBuilder<Collection> qb;
      qb.addTable( Resource::tableName() );
      qb.addValueCondition( Collection::remoteIdFullColumnName(), Query::Equals, rid );
      qb.addColumnCondition( Collection::resourceIdFullColumnName(), Query::Equals, Resource::idFullColumnName() );
      if ( mResource.isValid() )
        qb.addValueCondition( Resource::idFullColumnName(), Query::Equals, mResource.id() );
      else if ( connection()->resourceContext().isValid() )
        qb.addValueCondition( Resource::idFullColumnName(), Query::Equals, connection()->resourceContext().id() );
      else
        throw HandlerException( "Cannot retrieve collection based on remote identifier without a resource context" );
      if ( !qb.exec() )
        throw HandlerException( "Unable to retrieve collection for listing" );
      Collection::List results = qb.result();
      if ( results.count() != 1 )
        throw HandlerException( QByteArray::number( results.count() ) + " collections found" );
      col = results.first();
    } else
      throw HandlerException( "WTF" );
    if ( !col.isValid() )
      return failureResponse( "Collection " + QByteArray::number( baseCollection ) + " does not exist" );
    if ( depth == 0 )
      collections << col;
    else {
      collections << col.children();
      --depth;
    }
  } else {
    if ( depth != 0 ) {
      Collection::List list = Collection::retrieveFiltered( Collection::parentIdColumn(), QVariant() );
      collections << list;
    }
    --depth;
  }

  foreach ( const Collection &col, collections )
    listCollection( col, depth );

  Response response;
  response.setSuccess();
  response.setTag( tag() );
  response.setString( "List completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}

#include "aklist.moc"
