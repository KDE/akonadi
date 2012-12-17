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

#ifndef AKONADI_SOCIALUTILS_SOCIALFEEDITEM_H
#define AKONADI_SOCIALUTILS_SOCIALFEEDITEM_H

#include "libakonadisocialutils_export.h"

#include <QSharedPointer>
#include <QVariant>
#include <QUrl>

class KDateTime;

namespace Akonadi {

class SocialFeedItemPrivate;

/**
 * Class representing one entry in the social feed
 */
class LIBAKONADISOCIALUTILS_EXPORT SocialFeedItem
{
  public:
    SocialFeedItem();
    SocialFeedItem( const SocialFeedItem &other );
    ~SocialFeedItem();
    SocialFeedItem &operator=(const SocialFeedItem &other);

    /**
     * This returns the service string such as "on Facebook", "on Twitter"
     * It's used in the feed as the first line of the item
     * eg. "Marty on Twitter:"
     *
     * @return Network string
     */
    QString networkString() const;

    /**
     * Sets the network string for this item
     *
     * @param networkString The network string
     */
    void setNetworkString( const QString &networkString );

    /**
     * @return Original post id (eg. Facebook post id, Twitter post id etc).
     */
    QString postId() const;

    /**
     * Sets the original post id
     *
     * @param postId Post id from the network (eg. Facebook post id, Twitter post id etc).
     */
    void setPostId( const QString &postId );

    /**
     * @return The text of the post that is displayed in the feed
     */
    QString postText() const;

    /**
     * Sets the post text to be displayed in the feed
     *
     * @param text The post text in the feed
     */
    void setPostText( const QString &text );

    /**
     * @return Link from the post
     */
    QUrl postLink() const;

    /**
     * Sets the link the posts links to
     *
     * @param link URL of the link
     */
    void setPostLink( const QUrl &link );

    /**
     * @return Link title from the post
     */
    QString postLinkTitle() const;

    /**
     * Sets the link title the posts links to
     *
     * @param link Title of the link
     */
    void setPostLinkTitle( const QString &linkTitle );

    /**
     * @return URL of an image associated with this post
     */
    QUrl postImageUrl() const;

    /**
     * Sets the URL of an image associated with this post,
     * it can be an image thumb, link thumb etc.
     *
     * @param imageUrl The URL of the image
     */
    void setPostImageUrl( const QUrl &imageUrl );

    /**
     * @return Post user name
     */
    QString userName() const;

    /**
     * Sets the network user name associated with this post
     *
     * @param userName Network user name
     */
    void setUserName( const QString &userName );

    /**
     * @return Name that is displayed to the user
     */
    QString userDisplayName() const;

    /**
     * Sets the name to be displayed to the user (full name usually)
     *
     * @param userDisplayName Display name
     */
    void setUserDisplayName( const QString &userDisplayName );

    /**
     * @return Network user id
     */
    QString userId() const;

    /**
     * Sets the network user id associated with this post
     *
     * @param userId Network user id
     */
    void setUserId( const QString &userId );

    /**
     * @return Time of the post
     */
    KDateTime postTime() const;

    /**
     * Sets the time string which was received from the network together with the format
     * which could be received from the network as well or custom-supplied
     *
     * @param postTimeString The time string
     * @param postTimeFormat The time format of the string
     */
    void setPostTime( const QString &postTimeString, const QString &postTimeFormat );

    /**
     * @return The time format of the time string for this post
     */
    QString postTimeFormat() const;

    /**
     * @return The original time string as received from the network
     */
    QString postTimeString() const;

    /**
     * @return Additional info for the post, like number of comments, likes, retweed from etc
     */
    QString postInfo() const;

    /**
     * Sets additional info for the post, like number of comments, likes, retweed from etc
     *
     * For now all just in one string
     * @param postInfo The string with the info
     */
    void setPostInfo( const QString &postInfo );

    /**
     * @return True if this post was shared from other user
     */
    bool isShared() const;

    /**
     * Sets if this post was shared from other user
     *
     * @param shared True if shared, false if not
     */
    void setShared( bool shared );

    /**
     * @return Display name of the user which was this post shared from
     */
    QString sharedFrom() const;

    /**
     * Sets the display name of the user which was the original author of this post
     *
     * @param sharedFrom User which was this post shared from
     */
    void setSharedFrom( const QString &sharedFrom );

    /**
     * @return User id of the user this was shared from
     */
    QString sharedFromId() const;

    /**
     * Sets the user id of the user this was shared from
     *
     * @param sharedFromId User id of the original author
     */
    void setSharedFromId( const QString &sharedFromId );

    /**
     * Sets if the user has liked/favorited the post or not
     *
     * @param liked True if the post was liked/favorited, false otherwise
     */
    void setLiked( bool liked );

    /**
     * @return True if the post was liked/favorited by the signed-in user, false otherwise
     */
    bool isLiked() const;

    /**
     * @return The whole original data as received from the network mapped in QVariantMap
     */
    QVariantMap itemSourceMap() const;

    /**
     * Sets the original data which was received from the network and then mapped to a QVariantMap.
     * This allows the full original data to be serialized along with the social feed item
     * and reused later for whatever purpose
     *
     * @param itemSourceMap The original data in a QVariantMap
     *
     */
    void setItemSourceMap( const QVariantMap &itemSourceMap );

    /**
     * @return Url of the avatar picture
     */
    QUrl avatarUrl() const;

    /**
     * Sets the url of the avatar picture to be displayed next to the post in the feed
     *
     * @param url Avatar url
     */
    void setAvatarUrl( const QUrl &url );

    /**
     * Sets replies/comments for this post
     * @param replies List of replies
     */
    void setPostReplies( const QList<SocialFeedItem> &replies );

    /**
     * @return List of replies/comments to this post
     */
    QList<SocialFeedItem> postReplies() const;

  private:
    QSharedDataPointer<SocialFeedItemPrivate> d;
};

}

Q_DECLARE_METATYPE( Akonadi::SocialFeedItem )

#endif // SOCIALFEEDITEM_H
