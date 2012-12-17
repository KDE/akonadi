/*
  Social feed serializer
  Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or (at your
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

#include "akonadi_serializer_socialfeeditem.h"
#include "../socialfeeditem.h"

#include "akonadi/config-akonadi.h"
#include "akonadi/item.h"

#include <QtCore/qplugin.h>

#include <qjson/qobjecthelper.h>
#include <qjson/parser.h>
#include <qjson/serializer.h>

using namespace Akonadi;

bool SocialFeedItemSerializerPlugin::deserialize( Item &item,
                                                  const QByteArray &label,
                                                  QIODevice &data,
                                                  int version )
{
  Q_UNUSED( version );

  if ( label != Item::FullPayload ) {
    return false;
  }

  SocialFeedItem feedItem;

  QJson::Parser parser;
  QVariantMap map = parser.parse( data.readAll() ).toMap();

  feedItem.setNetworkString( map.value( QLatin1String( "networkString" ) ).toString() );
  feedItem.setPostId( map.value( QLatin1String( "postId" ) ).toString() );
  feedItem.setPostText( map.value( QLatin1String( "postText" ) ).toString() );
  feedItem.setPostLinkTitle( map.value( QLatin1String( "postLinkTitle" ) ).toString() );
  feedItem.setPostLink( map.value( QLatin1String( "postLink" ) ).toUrl() );
  feedItem.setPostImageUrl( map.value( QLatin1String( "postImageUrl" ) ).toUrl() );
  feedItem.setPostInfo( map.value( QLatin1String( "postInfo" ) ).toString() );
  feedItem.setUserName( map.value( QLatin1String( "userName" ) ).toString() );
  feedItem.setUserDisplayName( map.value( QLatin1String( "userDisplayName" ) ).toString() );
  feedItem.setUserId( map.value( QLatin1String( "userId" ) ).toString() );
  feedItem.setAvatarUrl( map.value( QLatin1String( "avatarUrl" ) ).toUrl() );
  feedItem.setPostTime( map.value( QLatin1String( "postTimeString" ) ).toString(),
                        map.value( QLatin1String( "postTimeFormat" ) ).toString() );
  feedItem.setShared( map.value( QLatin1String( "shared" ) ).toBool() );
  feedItem.setSharedFrom( map.value( QLatin1String( "sharedFrom" ) ).toString() );
  feedItem.setSharedFromId( map.value( QLatin1String( "sharedFromId" ) ).toString() );
  feedItem.setLiked( map.value( QLatin1String( "liked" ) ).toBool() );
  feedItem.setItemSourceMap( map.value( QLatin1String( "itemSourceMap" ) ).toMap() );

  if ( map.keys().contains( QLatin1String( "postReplies" ) ) ) {
    QList<SocialFeedItem> replies;
    Q_FOREACH ( const QVariant &replyData, map.value( QLatin1String( "postReplies" ) ).toList() ) {
      QVariantMap reply = replyData.toMap();
      SocialFeedItem postReply;
      postReply.setUserId( reply.value( QLatin1String( "userId" ) ).toString() );
      postReply.setUserName( reply.value( QLatin1String( "userName" ) ).toString() );
      postReply.setAvatarUrl( reply.value( QLatin1String( "userAvatarUrl" ) ).toString() );
      postReply.setPostText( reply.value( QLatin1String( "replyText" ) ).toString() );
//       postReply.setPostTime( reply.value( QLatin1String( "replyTime" ) ).toString();
      postReply.setPostId( reply.value( QLatin1String( "replyId" ) ).toString() );
//       postReply.postId        = reply.value( QLatin1String( "postId" ) ).toString();

      replies.append( postReply );
    }

    feedItem.setPostReplies( replies );
  }

  item.setMimeType( QLatin1String( "text/x-vnd.akonadi.socialfeeditem" ) );
  item.setPayload<SocialFeedItem>( feedItem );

  return true;
}

void SocialFeedItemSerializerPlugin::serialize( const Item &item,
                                                const QByteArray &label,
                                                QIODevice &data,
                                                int &version )
{
  Q_UNUSED( label );
  Q_UNUSED( version );

  if ( !item.hasPayload<SocialFeedItem>() ) {
    return;
  }

  SocialFeedItem feedItem = item.payload<SocialFeedItem>();

  QVariantMap map;

  map.insert( QLatin1String( "networkString" ), feedItem.networkString() );
  map.insert( QLatin1String( "postId" ), feedItem.postId() );
  map.insert( QLatin1String( "postText" ), feedItem.postText() );
  map.insert( QLatin1String( "postLinkTitle" ), feedItem.postLinkTitle() );
  map.insert( QLatin1String( "postLink" ), feedItem.postLink() );
  map.insert( QLatin1String( "postImageUrl" ), feedItem.postImageUrl() );
  map.insert( QLatin1String( "postInfo" ), feedItem.postInfo() );
  map.insert( QLatin1String( "userName" ), feedItem.userName() );
  map.insert( QLatin1String( "userDisplayName" ), feedItem.userDisplayName() );
  map.insert( QLatin1String( "userId" ), feedItem.userId() );
  map.insert( QLatin1String( "avatarUrl" ), feedItem.avatarUrl() );
  map.insert( QLatin1String( "postTimeString" ), feedItem.postTimeString() );
  map.insert( QLatin1String( "postTimeFormat" ), feedItem.postTimeFormat() );
  map.insert( QLatin1String( "shared" ), feedItem.isShared() );
  map.insert( QLatin1String( "sharedFrom" ), feedItem.sharedFrom() );
  map.insert( QLatin1String( "sharedFromId" ), feedItem.sharedFromId() );
  map.insert( QLatin1String( "liked" ), feedItem.isLiked() );
  map.insert( QLatin1String( "itemSourceMap" ), feedItem.itemSourceMap() );

  if (!feedItem.postReplies().isEmpty() ) {
    QVariantList replies;
    Q_FOREACH ( const SocialFeedItem &reply, feedItem.postReplies() ) {
      QVariantMap replyData;
      replyData.insert( QLatin1String( "userId" ), reply.userId() );
      replyData.insert( QLatin1String( "userName" ), reply.userName() );
      replyData.insert( QLatin1String( "userAvatarUrl" ), reply.avatarUrl() );
      replyData.insert( QLatin1String( "replyText" ), reply.postText() );
//       replyData.insert( QLatin1String( "replyTime" ), reply.postTimeString() );
      replyData.insert( QLatin1String( "replyId" ), reply.postId() );
//       replyData.insert( QLatin1String( "postId" ), reply.postId );
      replies.append( replyData );
    }

    map.insert( QLatin1String( "postReplies" ), replies );
  }

  QJson::Serializer serializer;
#if !defined( USE_QJSON_0_8 )
  data.write( serializer.serialize( map ) );
#else
  data.write( serializer.serialize( map, 0 ) );
#endif
}

QSet<QByteArray> SocialFeedItemSerializerPlugin::parts( const Item &item ) const
{
  // only need to reimplement this when implementing partial serialization
  // i.e. when using the "label" parameter of the other two methods
  return ItemSerializerPlugin::parts( item );
}

Q_EXPORT_PLUGIN2( akonadi_serializer_socialfeeditem, Akonadi::SocialFeedItemSerializerPlugin )

