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

#include "list.h"

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

List::List( Scope::SelectionScope scope, bool onlySubscribed ):
    Handler(),
    mScope( scope ),
    mAncestorDepth( 0 ),
    mOnlySubscribed( onlySubscribed ),
    mIncludeStatistics( false )
{}

QStack<Collection> List::ancestorsForCollection( const Collection &col )
{
  if ( mAncestorDepth <= 0 )
    return QStack<Collection>();
  QStack<Collection> ancestors;
  Collection parent = col.parent();
  for ( int i = 0; i < mAncestorDepth; ++i ) {
    if ( !parent.isValid() )
      break;
    ancestors.prepend( parent );
    parent = parent.parent();
  }
  return ancestors;
}

bool List::listCollection(const Collection & root, int depth, const QStack<Collection> &ancestors )
{
  // recursive listing of child collections
  bool childrenFound = false;
  if ( depth > 0 ) {
    Collection::List children = root.children();
    QStack<Collection> ancestorsAndMe( ancestors );
    ancestorsAndMe.push( root );
    foreach ( const Collection &col, children ) {
      if ( listCollection( col, depth - 1, ancestorsAndMe ) )
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
  const QByteArray b = HandlerHelper::collectionToByteArray( dummy, hidden, mIncludeStatistics, mAncestorDepth, ancestors );

  Response response;
  response.setUntagged();
  response.setString( b );
  emit responseAvailable( response );

  return true;
}

bool List::parseStream()
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

  int depth = HandlerHelper::parseDepth( m_streamParser->readString() );

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
        if ( mt.isValid() )
          mMimeTypes.append( mt.id() );
      }
    }
  }

  if ( m_streamParser->hasList() ) { // We got extra options
    m_streamParser->beginList();
    while ( !m_streamParser->atListEnd() ) {
      const QByteArray option = m_streamParser->readString();
      if ( option == "STATISTICS" ) {
        if ( m_streamParser->readString() == "true" )
          mIncludeStatistics = true;
      }
      if ( option == AKONADI_PARAM_ANCESTORS ) {
        const QByteArray argument = m_streamParser->readString();
        mAncestorDepth = HandlerHelper::parseDepth( argument );
      }
    }
  }

  Collection::List collections;
  QStack<Collection> ancestors;

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
    } else {
      throw HandlerException( "WTF" );
    }

    if ( !col.isValid() )
      return failureResponse( "Collection " + QByteArray::number( baseCollection ) + " does not exist" );

    if ( depth == 0 ) {
      collections << col;
      ancestors = ancestorsForCollection( col );
    } else {
      collections << col.children();
      --depth;
      if ( !collections.isEmpty() )
        ancestors = ancestorsForCollection( collections.first() );
    }
  } else {
    if ( depth != 0 ) {
      Collection::List list = Collection::retrieveFiltered( Collection::parentIdColumn(), QVariant() );
      collections << list;
    }
    --depth;
  }

  foreach ( const Collection &col, collections )
    listCollection( col, depth, ancestors );

  Response response;
  response.setSuccess();
  response.setTag( tag() );
  response.setString( "List completed" );
  emit responseAvailable( response );
  deleteLater();
  return true;
}

#include "list.moc"
