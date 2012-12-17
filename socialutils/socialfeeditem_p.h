/*
 * Copyright (C) 2012  Martin Klapetek <martin.klapetek@gmail.com>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef AKONADI_SOCIALUTILS_SOCIALFEEDITEM_P_H
#define AKONADI_SOCIALUTILS_SOCIALFEEDITEM_P_H

#include <QSharedData>
#include <QString>

#include <KDateTime>

#include "socialfeeditem.h"

class Akonadi::SocialFeedItemPrivate : public QSharedData
{
public:
  SocialFeedItemPrivate();
  SocialFeedItemPrivate( const Akonadi::SocialFeedItemPrivate &other );

  QVariantMap itemSourceMap;
  QString networkString;
  QString postId;
  QString postText;
  QUrl postLink;
  QString postLinkTitle;
  QUrl postImageUrl;
  QString userName;
  QString userDisplayName;
  QString userId;
  QString postTimeString;
  QString postTimeFormat;
  KDateTime postTime;
  QString postInfo;
  bool shared;
  QString sharedFrom;
  QString sharedFromId;
  QUrl avatarUrl;
  bool liked;
  QList<SocialFeedItem> replies;
};

Akonadi::SocialFeedItemPrivate::SocialFeedItemPrivate()
: QSharedData()
{
}

Akonadi::SocialFeedItemPrivate::SocialFeedItemPrivate( const Akonadi::SocialFeedItemPrivate &other )
: QSharedData( other )
{
}

#endif
