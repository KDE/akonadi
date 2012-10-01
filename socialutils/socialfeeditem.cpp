/*
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

#include "socialfeeditem.h"

#include <KDateTime>

#include <qjson/qobjecthelper.h>

class Akonadi::SocialFeedItemData : public QSharedData
{
public:
  SocialFeedItemData();
  SocialFeedItemData( const Akonadi::SocialFeedItemData &other );

  QVariantMap itemSourceMap;
  QString networkString;
  QString postId;
  QString postText;
  QString postLink;
  QString postLinkTitle;
  QString postImageUrl;
  QString userName;
  QString userDisplayName;
  QString userId;
  QString postTimeString;
  QString postTimeFormat;
  QString postInfo;
  bool shared;
  QString sharedFrom;
  QString sharedFromId;
  QUrl avatarUrl;
  bool liked;
  QList<PostReply> replies;
};

Akonadi::SocialFeedItemData::SocialFeedItemData()
  : QSharedData()
{
}

Akonadi::SocialFeedItemData::SocialFeedItemData (const Akonadi::SocialFeedItemData& other)
  : QSharedData(other)
{
}

//========================================================================================

Akonadi::SocialFeedItem::SocialFeedItem()
  : d(new SocialFeedItemData)
{
}

Akonadi::SocialFeedItem::SocialFeedItem( const Akonadi::SocialFeedItem &other )
{
  d = other.d;
}


Akonadi::SocialFeedItem::~SocialFeedItem()
{
}

QString Akonadi::SocialFeedItem::networkString() const
{
  return d->networkString;
}

void Akonadi::SocialFeedItem::setNetworkString( const QString &networkString )
{
  d->networkString = networkString;
}

QString Akonadi::SocialFeedItem::postId() const
{
  return d->postId;
}

void Akonadi::SocialFeedItem::setPostId( const QString &postId )
{
  d->postId = postId;
}

QString Akonadi::SocialFeedItem::postText() const
{
  return d->postText;
}

void Akonadi::SocialFeedItem::setPostText( const QString &postText )
{
  d->postText = postText;
}

QString Akonadi::SocialFeedItem::postLink() const
{
  return d->postLink;
}

void Akonadi::SocialFeedItem::setPostLink( const QString &link )
{
  d->postLink = link;
}

QString Akonadi::SocialFeedItem::postLinkTitle() const
{
  return d->postLinkTitle;
}

void Akonadi::SocialFeedItem::setPostLinkTitle( const QString &linkTitle )
{
  d->postLinkTitle = linkTitle;
}

QString Akonadi::SocialFeedItem::postImageUrl() const
{
  return d->postImageUrl;
}

void Akonadi::SocialFeedItem::setPostImageUrl( const QString &imageUrl )
{
  d->postImageUrl = imageUrl;
}

KDateTime Akonadi::SocialFeedItem::postTime() const
{
  return KDateTime::fromString( d->postTimeString, d->postTimeFormat );
}


QString Akonadi::SocialFeedItem::postTimeString() const
{
  return d->postTimeString;
}

QString Akonadi::SocialFeedItem::postInfo() const
{
  return d->postInfo;
}

void Akonadi::SocialFeedItem::setPostInfo( const QString &postInfo )
{
  d->postInfo = postInfo;
}

void Akonadi::SocialFeedItem::setPostTime( const QString &postTimeString )
{
  d->postTimeString = postTimeString;
}

void Akonadi::SocialFeedItem::setPostTime( const QString &postTimeString, const QString &postTimeFormat )
{
  d->postTimeString = postTimeString;
  d->postTimeFormat = postTimeFormat;
}

QString Akonadi::SocialFeedItem::postTimeFormat() const
{
  return d->postTimeFormat;
}

void Akonadi::SocialFeedItem::setPostTimeFormat( const QString &postTimeFormat )
{
  d->postTimeFormat = postTimeFormat;
}

QString Akonadi::SocialFeedItem::userId() const
{
  return d->userId;
}

void Akonadi::SocialFeedItem::setUserId( const QString &userId )
{
  d->userId = userId;
}

QString Akonadi::SocialFeedItem::userName() const
{
  return d->userName;
}

void Akonadi::SocialFeedItem::setUserName( const QString &userName )
{
  d->userName = userName;
}

QString Akonadi::SocialFeedItem::userDisplayName() const
{
  return d->userDisplayName;
}

void Akonadi::SocialFeedItem::setUserDisplayName ( const QString& userDisplayName )
{
  d->userDisplayName = userDisplayName;
}

bool Akonadi::SocialFeedItem::isShared() const
{
  return d->shared;
}

void Akonadi::SocialFeedItem::setShared( bool shared )
{
  d->shared = shared;
}

QString Akonadi::SocialFeedItem::sharedFrom() const
{
  return d->sharedFrom;
}

void Akonadi::SocialFeedItem::setSharedFrom( const QString &sharedFrom )
{
  d->sharedFrom = sharedFrom;
}

QString Akonadi::SocialFeedItem::sharedFromId() const
{
  return d->sharedFromId;
}

void Akonadi::SocialFeedItem::setSharedFromId( const QString &sharedFromId )
{
  d->sharedFromId = sharedFromId;
}

QVariantMap Akonadi::SocialFeedItem::itemSourceMap() const
{
  return d->itemSourceMap;
}

void Akonadi::SocialFeedItem::setItemSourceMap( const QVariantMap &itemSourceMap )
{
  d->itemSourceMap = itemSourceMap;
}

QUrl Akonadi::SocialFeedItem::avatarUrl() const
{
  return d->avatarUrl;
}

void Akonadi::SocialFeedItem::setAvatarUrl( const QUrl &url )
{
  d->avatarUrl = url;
}

bool Akonadi::SocialFeedItem::isLiked() const
{
  return d->liked;
}

void Akonadi::SocialFeedItem::setLiked( bool liked )
{
  d->liked = liked;
}

QList<Akonadi::PostReply> Akonadi::SocialFeedItem::postReplies() const
{
  return d->replies;
}

void Akonadi::SocialFeedItem::setPostReplies( const QList<Akonadi::PostReply> &replies )
{
  d->replies = replies;
}
