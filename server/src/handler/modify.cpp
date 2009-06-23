/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

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

#include "modify.h"

#include <akonadiconnection.h>
#include <storage/datastore.h>
#include <storage/entity.h>
#include <storage/transaction.h>
#include "libs/imapparser_p.h"
#include "imapstreamparser.h"
#include <handlerhelper.h>
#include <response.h>
#include <storage/itemretriever.h>
#include <storage/selectquerybuilder.h>
#include <storage/collectionqueryhelper.h>
#include <libs/protocol_p.h>

using namespace Akonadi;

Modify::Modify( Scope::SelectionScope scope ) : m_scope( scope )
{
}

bool Modify::parseStream()
{
  m_scope.parseScope( m_streamParser );
  SelectQueryBuilder<Collection> qb;
  CollectionQueryHelper::scopeToQuery( m_scope, connection(), qb );
  if ( !qb.exec() )
    throw HandlerException( "Unable to execute collection query" );
  const Collection::List collections = qb.result();
  if ( collections.isEmpty() )
    throw HandlerException( "No such collection" );
  if ( collections.size() > 1 ) // TODO
    throw HandlerException( "Mass-modifying collections is not yet implemented" );
  Collection collection = collections.first();


  //TODO: do it cleanly with the streaming parser, which doesn't have look-ahead at this moment
  QByteArray line = m_streamParser->readUntilCommandEnd();
  m_streamParser->insertData( "\n" );

  int p = 0;
  if ( (p = line.indexOf( "PARENT ")) > 0 ) {
    QByteArray tmp;
    ImapParser::parseString( line, tmp, p + 6 );
    const Collection newParent = HandlerHelper::collectionFromIdOrName( tmp );
    if ( newParent.isValid() && collection.parentId() != newParent.id()
         && collection.resourceId() != newParent.resourceId() )
    {
      ItemRetriever retriever( connection() );
      retriever.setCollection( collection, true );
      retriever.setRetrieveFullPayload( true );
      retriever.exec();
    }
  }

  DataStore *db = connection()->storageBackend();
  Transaction transaction( db );
  QList<QByteArray> changes;

  int pos = 0;
  while ( pos < line.length() ) {
    QByteArray type;
    pos = ImapParser::parseString( line, type, pos );
    if ( type == AKONADI_PARAM_MIMETYPE ) {
      QList<QByteArray> mimeTypes;
      pos = ImapParser::parseParenthesizedList( line, mimeTypes, pos );
      QStringList mts;
      foreach ( const QByteArray &ba, mimeTypes )
        mts << QString::fromLatin1(ba);
      MimeType::List currentMts = collection.mimeTypes();
      bool equal = true;
      foreach ( const MimeType &currentMt, currentMts ) {
        if ( mts.contains( currentMt.name() ) ) {
          mts.removeAll( currentMt.name() );
          continue;
        }
        equal = false;
        if ( !collection.removeMimeType( currentMt ) )
          throw HandlerException( "Unable to remove collection mimetype" );
      }
      if ( !db->appendMimeTypeForCollection( collection.id(), mts ) )
        return failureResponse( "Unable to add collection mimetypes" );
      if ( !equal || !mts.isEmpty() )
        changes.append( AKONADI_PARAM_MIMETYPE );
    } else if ( type == AKONADI_PARAM_CACHEPOLICY ) {
      bool changed = false;
      pos = HandlerHelper::parseCachePolicy( line, collection, pos, &changed );
      if ( changed )
        changes.append( AKONADI_PARAM_CACHEPOLICY );
    } else if ( type == AKONADI_PARAM_NAME ) {
      QByteArray newName;
      pos = ImapParser::parseString( line, newName, pos );
      if ( collection.name() == newName )
        continue;
      if ( !db->renameCollection( collection, collection.parentId(), newName ) )
        return failureResponse( "Unable to rename collection" );
      changes.append( AKONADI_PARAM_NAME );
    } else if ( type == "PARENT" ) {
      qint64 newParent;
      bool ok = false;
      pos = ImapParser::parseNumber( line, newParent, &ok, pos );
      if ( !ok )
        return failureResponse( "Invalid syntax" );
      if ( collection.parentId() == newParent )
        continue;
      if ( !db->renameCollection( collection, newParent, collection.name() ) )
        return failureResponse( "Unable to reparent collection" );
      changes.append( "PARENT" );
    } else if ( type == AKONADI_PARAM_REMOTEID ) {
      QString rid;
      pos = ImapParser::parseString( line, rid, pos );
      if ( rid == collection.remoteId() )
        continue;
      collection.setRemoteId( rid );
      changes.append( AKONADI_PARAM_REMOTEID );
    } else if ( type.isEmpty() ) {
      break; // input end
    } else {
      // custom attribute
      if ( type.startsWith( '-' ) ) {
        type = type.mid( 1 );
        if ( !db->removeCollectionAttribute( collection, type ) )
          return failureResponse( "Unable to remove custom collection attribute" );
        changes.append( type );
      } else {
        QByteArray value;
        pos = ImapParser::parseString( line, value, pos );

        SelectQueryBuilder<CollectionAttribute> qb;
        qb.addValueCondition( CollectionAttribute::collectionIdColumn(), Query::Equals, collection.id() );
        qb.addValueCondition( CollectionAttribute::typeColumn(), Query::Equals, type );
        if ( !qb.exec() )
          throw HandlerException( "Unable to retrieve collection attribute" );

        const CollectionAttribute::List attrs = qb.result();
        if ( attrs.isEmpty() ) {
          CollectionAttribute attr;
          attr.setCollectionId( collection.id() );
          attr.setType( type );
          attr.setValue( value );
          if ( !attr.insert() )
            throw HandlerException( "Unable to add collection attribute" );
          changes.append( type );
        } else if ( attrs.size() == 1 ) {
          CollectionAttribute attr = attrs.first();
          if ( attr.value() == value )
            continue;
          attr.setValue( value );
          if ( !attr.update() )
            throw HandlerException( "Unable to update collection attribute" );
          changes.append( type );
        } else {
          throw HandlerException( "WTF: more than one attribute with the same name" );
        }
      }
    }
  }

  if ( !changes.isEmpty() ) {
    if ( collection.hasPendingChanges() && !collection.update() )
      return failureResponse( "Unable to update collection" );
    db->notificationCollector()->collectionChanged( collection, changes );
    if ( !transaction.commit() )
      return failureResponse( "Unable to commit transaction" );
  }

  Response response;
  response.setTag( tag() );
  response.setString( "MODIFY done" );
  emit responseAvailable( response );
  return true;
}

#include "modify.moc"
