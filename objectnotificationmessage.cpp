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

#include "objectnotificationmessage.h"

using namespace Akonadi;

ObjectNotificationMessage::ObjectNotificationMessage(const Akonadi::NotificationMessage &message)
  : m_parentCollection(message.parentCollection()),
    m_parentDestCollection(message.parentDestCollection()), 
    m_message(message)
{
  if (message.type() == NotificationMessage::Collection) {
    Collection col( message.uid() );
    col.setResource( QString::fromUtf8( message.resource() ) );
    col.setRemoteId( message.remoteId() );
    col.setParentCollection( message.operation() == NotificationMessage::Move ? m_parentDestCollection : m_parentCollection );

    if ( message.operation() == NotificationMessage::Remove ) {
      const QString remoteRevision = QString::fromUtf8( message.itemParts().toList().first() );
      col.setRemoteRevision( remoteRevision );
    }
    m_collections.append( col );
  } else if (message.type() == NotificationMessage::Item) {
    Item item( message.uid() );
    item.setRemoteId( message.remoteId() );
    item.setMimeType( message.mimeType() );
    item.setParentCollection( message.operation() == NotificationMessage::Move ? m_parentDestCollection : m_parentCollection );

    // HACK: We have the remoteRevision stored in the itemParts set
    //       for delete operations to avoid protocol breakage
    if ( message.operation() == NotificationMessage::Remove ) {
      const QString remoteRevision = QString::fromUtf8( message.itemParts().toList().first() );
      item.setRemoteRevision( remoteRevision );
    }

    m_items.append( item );
  }

  // HACK: destination resource is delivered in the parts field...
  if ( message.operation() == NotificationMessage::Move && !message.itemParts().isEmpty() )
    m_parentDestCollection.setResource( QString::fromLatin1( *(message.itemParts().begin()) ) );

  m_parentCollection.setResource( QString::fromUtf8( message.resource() ) );
}

Item::List ObjectNotificationMessage::items() const
{
  return m_items;
}

Collection::List ObjectNotificationMessage::collections() const
{
  return m_collections;
}

NotificationMessage ObjectNotificationMessage::message() const
{
  return m_message;
}

NotificationMessage::Operation ObjectNotificationMessage::operation() const
{
  return m_message.operation();
}

NotificationMessage::Type ObjectNotificationMessage::type() const
{
  return m_message.type();
}

QByteArray ObjectNotificationMessage::resource() const
{
  return m_message.resource();
}

QString ObjectNotificationMessage::remoteId() const
{
  return m_message.remoteId();
}

QSet<QByteArray> ObjectNotificationMessage::itemParts() const
{
  return m_message.itemParts();
}

QString ObjectNotificationMessage::mimeType() const
{
  return m_message.mimeType();
}

Collection ObjectNotificationMessage::parentCollection() const
{
  return m_parentCollection;
}

Collection ObjectNotificationMessage::parentDestCollection() const
{
  return m_parentDestCollection;
}

void ObjectNotificationMessage::appendCollections(const Akonadi::Collection::List& list)
{
  m_collections.append(list);
}

void ObjectNotificationMessage::appendItems(const Akonadi::Item::List& list)
{
  m_items.append(list);
}

template<typename Container>
bool do_appendAndCompress(Container &container, const Akonadi::ObjectNotificationMessage& message )
{
  if ( container.isEmpty() ) {
    container.push_back(message);
    return true;
  }
  const ObjectNotificationMessage lastMessage = container.last().message();
  if (lastMessage.message().type() == message.message().type()
      && lastMessage.message().operation() == message.message().operation()
      && lastMessage.parentCollection() == message.parentCollection()
      && lastMessage.parentDestCollection() == message.parentDestCollection() ) {
    container.last().appendCollections(message.collections());
    container.last().appendItems(message.items());
  } else {
    container.push_back(message);
    return true;
  }
  return false;
}

bool ObjectNotificationMessage::appendAndCompress(QVector<ObjectNotificationMessage> &vector, const Akonadi::ObjectNotificationMessage& message )
{
  return do_appendAndCompress(vector, message);
}

bool ObjectNotificationMessage::appendAndCompress(QList<ObjectNotificationMessage> &list, const Akonadi::ObjectNotificationMessage& message )
{
  return do_appendAndCompress(list, message);
}
