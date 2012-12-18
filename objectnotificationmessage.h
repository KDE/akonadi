/*
    Copyright (c) 2011 Stephen Kelly <steveire@gmail.com>

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

#ifndef AKONADI_OBJECTNOTIFICATIONMESSAGE_H
#define AKONADI_OBJECTNOTIFICATIONMESSAGE_H

#include "collection.h"
#include "item.h"
#include "notificationmessage_p.h"

#include <QQueue>

namespace Akonadi
{

class NotificationMessage;

/**
 * @brief This class wraps a group of equivalent NotificationMessages from Akonadi
 *
 * The primary feature is that bulk notifications of multiple items or collections can
 * be represented with one ObjectNotificationMessage.
 */
class ObjectNotificationMessage
{
public:
  ObjectNotificationMessage(const Akonadi::NotificationMessage &message = Akonadi::NotificationMessage()); // krazy:exclude=explicit

  static bool appendAndCompress( QList<ObjectNotificationMessage> &vector, const Akonadi::ObjectNotificationMessage &message);
  static bool appendAndCompress( QVector<ObjectNotificationMessage> &vector, const Akonadi::ObjectNotificationMessage &message);

  Item::List items() const;
  Collection::List collections() const;
  Collection parentCollection() const;
  Collection parentDestCollection() const;
  NotificationMessage message() const;
  NotificationMessage::Operation operation() const;
  NotificationMessage::Type type() const;
  QByteArray resource() const;
  QString remoteId() const;
  QSet<QByteArray> itemParts() const;
  QString mimeType() const;

  void appendItems(const Item::List &list);
  void appendCollections(const Collection::List &list);

private:
  Item::List m_items;
  Collection::List m_collections;
  Collection m_parentCollection;
  Collection m_parentDestCollection;
  NotificationMessage m_message;
};

}

Q_DECLARE_TYPEINFO(Akonadi::ObjectNotificationMessage, Q_MOVABLE_TYPE);

#endif
