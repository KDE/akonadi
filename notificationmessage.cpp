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

#include "notificationmessage.h"

#include <QDBusMetaType>
#include <QDebug>
#include <QHash>

using namespace Akonadi;

class NotificationMessage::Private : public QSharedData
{
  public:
    Private() : QSharedData(),
      type( NotificationMessage::InvalidType ),
      operation( NotificationMessage::InvalidOp ),
      uid( -1 ),
      parentCollection( -1 )
    {}

    Private( const Private &other ) : QSharedData( other )
    {
      sessionId = other.sessionId;
      type = other.type;
      operation = other.operation;
      uid = other.uid;
      remoteId = other.remoteId;
      resource = other.resource;
      parentCollection = other.parentCollection;
      mimeType = other.mimeType;
      parts = other.parts;
    }

    bool compareWithoutOp( const Private &other ) const
    {
      return sessionId == other.sessionId
          && type == other.type
          && uid == other.uid
          && remoteId == other.remoteId
          && resource == other.resource
          && parentCollection == other.parentCollection
          && mimeType == other.mimeType
          && parts == other.parts;
    }

    bool operator==(const Private &other ) const
    {
      return operation == other.operation && compareWithoutOp( other );
    }

    QByteArray sessionId;
    NotificationMessage::Type type;
    NotificationMessage::Operation operation;
    int uid;
    QString remoteId;
    QByteArray resource;
    int parentCollection;
    QString mimeType;
    QStringList parts;
};

NotificationMessage::NotificationMessage() :
    d( new Private )
{
}

NotificationMessage::NotificationMessage(const NotificationMessage & other) :
    d( other.d )
{
}

NotificationMessage::~ NotificationMessage()
{
}

NotificationMessage & NotificationMessage::operator =(const NotificationMessage & other)
{
  if ( this != &other )
    d = other.d;
  return *this;
}

bool NotificationMessage::operator ==(const NotificationMessage & other) const
{
  return d == other.d;
}

void NotificationMessage::registerDBusTypes()
{
  qDBusRegisterMetaType<Akonadi::NotificationMessage>();
  qDBusRegisterMetaType<Akonadi::NotificationMessage::List>();
}

QByteArray NotificationMessage::sessionId() const
{
  return d->sessionId;
}

void NotificationMessage::setSessionId( const QByteArray &sessionId )
{
  d->sessionId = sessionId;
}

NotificationMessage::Type NotificationMessage::type() const
{
  return d->type;
}

void NotificationMessage::setType(Type type)
{
  d->type = type;
}

NotificationMessage::Operation NotificationMessage::operation() const
{
  return d->operation;
}

void NotificationMessage::setOperation(Operation op)
{
  d->operation = op;
}

int NotificationMessage::uid() const
{
  return d->uid;
}

void NotificationMessage::setUid(int uid)
{
  d->uid = uid;
}

QString NotificationMessage::remoteId() const
{
  return d->remoteId;
}

void NotificationMessage::setRemoteId(const QString & rid)
{
  d->remoteId = rid;
}

QByteArray NotificationMessage::resource() const
{
  return d->resource;
}

void NotificationMessage::setResource(const QByteArray & res)
{
  d->resource = res;
}

int NotificationMessage::parentCollection() const
{
  return d->parentCollection;
}

void NotificationMessage::setParentCollection(int parent)
{
  d->parentCollection = parent;
}

QString NotificationMessage::mimeType() const
{
  return d->mimeType;
}

void NotificationMessage::setMimeType(const QString & mimeType)
{
  d->mimeType = mimeType;
}

QStringList NotificationMessage::itemParts() const
{
  return d->parts;
}

void NotificationMessage::setItemParts(const QStringList & parts)
{
  d->parts = parts;
}

QString NotificationMessage::toString() const
{
  QString rv;
  switch ( type() ) {
    case Item:
      rv += QLatin1String( "Item " );
      break;
    case Collection:
      rv += QLatin1String( "Collection " );
      break;
  }
  rv += QString::fromLatin1( "(%1, %2) " ).arg( uid() ).arg( remoteId() );
  if ( parentCollection() >= 0 )
    rv += QString::fromLatin1( "in collection %1 " ).arg( parentCollection() );
  switch ( operation() ) {
    case Add:
      rv += QLatin1String( "added" );
      break;
    case Modify:
      rv += QLatin1String( "modified" );
      break;
    case Remove:
      rv += QLatin1String( "removed" );
      break;
  }
  return rv;
}

void NotificationMessage::appendAndCompress(NotificationMessage::List & list, const NotificationMessage & msg)
{
  for ( NotificationMessage::List::Iterator it = list.begin(); it != list.end(); ) {
    if ( msg.d->compareWithoutOp( *((*it).d) ) ) {
      if ( msg.operation() == (*it).operation() || msg.operation() == Modify ) {
        return;
      }
      if ( msg.operation() == Remove && (*it).operation() == Modify ) {
        it = list.erase( it );
      } else
        ++it;
    } else
      ++it;
  }
  list << msg;
}

QDBusArgument & operator <<(QDBusArgument & arg, const NotificationMessage & msg)
{
  arg.beginStructure();
  arg << msg.sessionId();
  arg << msg.type();
  arg << msg.operation();
  arg << msg.uid();
  arg << msg.remoteId();
  arg << msg.resource();
  arg << msg.parentCollection();
  arg << msg.mimeType();
  arg << msg.itemParts();
  arg.endStructure();
  return arg;
}

const QDBusArgument & operator >>(const QDBusArgument & arg, NotificationMessage & msg)
{
  arg.beginStructure();
  QByteArray b;
  arg >> b;
  msg.setSessionId( b );
  int i;
  arg >> i;
  msg.setType( static_cast<NotificationMessage::Type>( i ) );
  arg >> i;
  msg.setOperation( static_cast<NotificationMessage::Operation>( i ) );
  arg >> i;
  msg.setUid( i );
  QString s;
  arg >> s;
  msg.setRemoteId( s );
  arg >> b;
  msg.setResource( b );
  arg >> i;
  msg.setParentCollection( i );
  arg >> s;
  msg.setMimeType( s );
  QStringList l;
  arg >> l;
  msg.setItemParts( l );
  arg.endStructure();
  return arg;
}

uint qHash(const Akonadi::NotificationMessage & msg)
{
  return qHash( msg.uid() + (msg.type() << 31) + (msg.operation() << 28) );
}
