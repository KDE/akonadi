/*
    Copyright (c) 2014 Daniel Vrátil <dvratil@redhat.com>

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

#include "tagstore.h"
#include "scope.h"
#include "tagfetchhelper.h"
#include "imapstreamparser.h"
#include "response.h"
#include "storage/datastore.h"
#include <storage/querybuilder.h>
#include "libs/protocol_p.h"
#include "connection.h"

using namespace Akonadi::Server;

TagStore::TagStore()
  : Handler()
{
}

TagStore::~TagStore()
{
}

bool TagStore::parseStream()
{
  const qint64 tagId = m_streamParser->readNumber();

  if ( !m_streamParser->hasList() ) {
    failureResponse( "No changes to store" );
    return false;
  }

  Tag changedTag = Tag::retrieveById( tagId );
  if ( !changedTag.isValid() ) {
    throw HandlerException( "No such tag" );
  }

  // Retrieve all tag's attributes
  const TagAttribute::List attributes = TagAttribute::retrieveFiltered( TagAttribute::tagIdFullColumnName(), tagId );
  QMap<QByteArray,TagAttribute> attributesMap;
  Q_FOREACH ( const TagAttribute &attribute, attributes ) {
    attributesMap.insert( attribute.type(), attribute );
  }

  bool tagRemoved = false;
  m_streamParser->beginList();
  while ( !m_streamParser->atListEnd() ) {
    const QByteArray attr = m_streamParser->readString();

    if ( attr == AKONADI_PARAM_PARENT ) {
      const qint64 parent = m_streamParser->readNumber();
            changedTag.setParentId( parent );
    } else if ( attr == AKONADI_PARAM_GID ) {
      throw HandlerException( "Changing tag GID is not allowed" );
    } else if ( attr == AKONADI_PARAM_UID ) {
      throw HandlerException( "Changing tag UID is not allowed" );
    } else if ( attr == AKONADI_PARAM_MIMETYPE ) {
        const QString &typeName = m_streamParser->readUtf8String();
        TagType type = TagType::retrieveByName(typeName);
        if (!type.isValid()) {
            TagType newType;
            newType.setName(typeName);
            if (!newType.insert()) {
                throw HandlerException( "Failed to insert type" );
            }
            type = newType;
        }
        changedTag.setTagType(type);
    } else if ( attr == AKONADI_PARAM_REMOTEID ) {
        const QString &remoteId = m_streamParser->readUtf8String();
        if (!connection()->context()->resource().isValid()) {
            throw HandlerException( "Only resources can change the remoteid" );
        }
        //Simply using remove() doesn't work since we need two arguments
        QueryBuilder qb( TagRemoteIdResourceRelation::tableName(), QueryBuilder::Delete );
        qb.addValueCondition( TagRemoteIdResourceRelation::tagIdColumn(), Query::Equals, tagId );
        qb.addValueCondition( TagRemoteIdResourceRelation::resourceIdColumn(), Query::Equals,connection()->context()->resource().id());
        qb.exec();

        if (!remoteId.isEmpty()) {
            TagRemoteIdResourceRelation remoteIdRelation;
            remoteIdRelation.setRemoteId(remoteId);
            remoteIdRelation.setResourceId(connection()->context()->resource().id());
            remoteIdRelation.setTag(changedTag);
            if (!remoteIdRelation.insert()) {
                throw HandlerException( "Failed to insert remotedid resource relation" );
            }
        } else {
            const int tagRidsCount = TagRemoteIdResourceRelation::count(TagRemoteIdResourceRelation::tagIdColumn(), changedTag.id());
            // We just removed the last RID of the tag, which means that no other
            // resource owns this tag, so we have to remove it to simulate tag
            // removal
            if (tagRidsCount == 0) {
                if (!DataStore::self()->removeTags(Tag::List() << changedTag)) {
                    throw HandlerException( "Failed to remove tag" );
                }
                tagRemoved = true;

                // We can't break here, we need to finish reading the command
            }
        }
    } else {
      if ( attr.startsWith( '-' ) ) {
        const QByteArray attrName = attr.mid( 1 );
        if ( attributesMap.contains( attrName ) ) {
          TagAttribute attribute = attributesMap.value( attrName );
          TagAttribute::remove( attribute.id() );
        }
      } else if ( attributesMap.contains( attr ) ) {
        const QByteArray value = m_streamParser->readString();
        TagAttribute attribute = attributesMap.value( attr );
        attribute.setValue( value );
        if (!attribute.update()) {
            throw HandlerException("Failed to update attribute");
        }
      } else {
        const QByteArray value = m_streamParser->readString();
        TagAttribute attribute;
        attribute.setTagId( tagId );
        attribute.setType( attr );
        attribute.setValue( value );
        if (!attribute.insert()) {
            throw HandlerException("Failed to insert attribute");
        }
      }
    }
  }

  if (!tagRemoved) {
      if (!changedTag.update()) {
        throw HandlerException( "Failed to store changes" );
      }
      DataStore::self()->notificationCollector()->tagChanged( changedTag );

      ImapSet set;
      set.add( QVector<qint64>() << tagId );
      TagFetchHelper helper( connection(), set );
      connect( &helper, SIGNAL(responseAvailable(Akonadi::Server::Response)),
               this, SIGNAL(responseAvailable(Akonadi::Server::Response)) );
      if ( !helper.fetchTags( AKONADI_CMD_TAGFETCH ) ) {
        return false;
      }
  } else {
      QByteArray str = QByteArray::number( tagId ) + " TAGREMOVE";
      Response response;
      response.setUntagged();
      response.setString( str );
      Q_EMIT responseAvailable( response );
  }

  Response response;
  response.setTag( tag() );
  response.setSuccess();
  response.setString( "TAGSTORE completed" );
  Q_EMIT responseAvailable( response );
  return true;
}
