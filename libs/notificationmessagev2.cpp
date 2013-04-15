/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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


#include "notificationmessagev2_p.h"
#include "notificationmessage_p.h"
#include "imapparser_p.h"

#include <QtCore/QDebug>
#include <QtCore/QHash>
#include <QtDBus/QDBusMetaType>
#include <qdbusconnection.h>

using namespace Akonadi;

class NotificationMessageV2::Private: public QSharedData
{
  public:
    Private()
      : QSharedData(),
        type( InvalidType ),
        operation( InvalidOp ),
        parentCollection( -1 ),
        parentDestCollection( -1 )
    {
    }

    Private( const Private &other )
      : QSharedData( other )
    {
      sessionId = other.sessionId;
      type = other.type;
      operation = other.operation;
      items = other.items;
      resource = other.resource;
      destResource = other.destResource;
      parentCollection = other.parentCollection;
      parentDestCollection = other.parentDestCollection;
      parts = other.parts;
      addedFlags = other.addedFlags;
      removedFlags = other.removedFlags;
    }

    bool compareWithoutOpAndParts( const Private &other ) const
    {
      return items == other.items
          && type == other.type
          && sessionId == other.sessionId
          && resource == other.resource
          && destResource == other.destResource
          && parentCollection == other.parentCollection
          && parentDestCollection == other.parentDestCollection;
    }

    bool operator==( const Private &other ) const
    {
      return operation == other.operation && parts == other.parts &&
             addedFlags == other.addedFlags && removedFlags == other.removedFlags &&
             compareWithoutOpAndParts( other );
    }


    QByteArray sessionId;
    NotificationMessageV2::Type type;
    NotificationMessageV2::Operation operation;
    QMap<Id, NotificationMessageV2::Item> items;
    QByteArray resource;
    QByteArray destResource;
    Id parentCollection;
    Id parentDestCollection;
    QSet<QByteArray> parts;
    QSet<QByteArray> addedFlags;
    QSet<QByteArray> removedFlags;
};

NotificationMessageV2::NotificationMessageV2():
  d( new Private )
{
}

NotificationMessageV2::NotificationMessageV2( const NotificationMessageV2 &other ):
  d( other.d )
{
}

NotificationMessageV2::~NotificationMessageV2()
{
}

NotificationMessageV2& NotificationMessageV2::operator=( const NotificationMessageV2 &other )
{
  if ( this != &other ) {
    d = other.d;
  }

  return *this;
}

bool NotificationMessageV2::operator==( const NotificationMessageV2 &other ) const
{
  return d == other.d;
}

void NotificationMessageV2::registerDBusTypes()
{
  qDBusRegisterMetaType<Akonadi::NotificationMessageV2>();
  qDBusRegisterMetaType<Akonadi::NotificationMessageV2::Item>();
  qDBusRegisterMetaType<Akonadi::NotificationMessageV2::List>();
}

void NotificationMessageV2::addEntity( Id id, const QString &remoteId, const QString &remoteRevision, const QString &mimeType )
{
  NotificationMessageV2::Item item;
  item.id = id;
  item.remoteId = remoteId;
  item.remoteRevision = remoteRevision;
  item.mimeType = mimeType;

  d->items.insert( id, item );
}

void NotificationMessageV2::setEntities( const QList<NotificationMessageV2::Item> &items )
{
  d->items.clear();
  Q_FOREACH( const NotificationMessageV2::Item &item, items ) {
    d->items.insert( item.id, item );
  }
}

QMap<NotificationMessageV2::Id, NotificationMessageV2::Item> NotificationMessageV2::entities() const
{
  return d->items;
}

NotificationMessageV2::Item NotificationMessageV2::entity( NotificationMessageV2::Id id ) const
{
  return d->items.value( id );
}

QList<NotificationMessageV2::Id> NotificationMessageV2::uids() const
{
  return d->items.keys();
}

QByteArray NotificationMessageV2::sessionId() const
{
  return d->sessionId;
}

void NotificationMessageV2::setSessionId( const QByteArray &sessionId )
{
  d->sessionId = sessionId;
}

NotificationMessageV2::Type NotificationMessageV2::type() const
{
  return d->type;
}

void NotificationMessageV2::setType( Type type )
{
  d->type = type;
}

NotificationMessageV2::Operation NotificationMessageV2::operation() const
{
  return d->operation;
}

void NotificationMessageV2::setOperation( Operation operation )
{
  d->operation = operation;
}

QByteArray NotificationMessageV2::resource() const
{
  return d->resource;
}

void NotificationMessageV2::setResource( const QByteArray &resource )
{
  d->resource = resource;
}

NotificationMessageV2::Id NotificationMessageV2::parentCollection() const
{
  return d->parentCollection;
}

NotificationMessageV2::Id NotificationMessageV2::parentDestCollection() const
{
  return d->parentDestCollection;
}

void NotificationMessageV2::setParentCollection( Id parent )
{
  d->parentCollection = parent;
}

void NotificationMessageV2::setParentDestCollection( Id parent )
{
  d->parentDestCollection = parent;
}

void NotificationMessageV2::setDestinationResource( const QByteArray &destResource )
{
  d->destResource = destResource;
}

QByteArray NotificationMessageV2::destinationResource() const
{
  return d->destResource;
}

QSet<QByteArray> NotificationMessageV2::itemParts() const
{
  return d->parts;
}

void NotificationMessageV2::setItemParts( const QSet<QByteArray> &parts )
{
  d->parts = parts;
}

QSet<QByteArray> NotificationMessageV2::addedFlags() const
{
  return d->addedFlags;
}

void NotificationMessageV2::setAddedFlags( const QSet<QByteArray> &addedFlags )
{
  d->addedFlags = addedFlags;
}

QSet<QByteArray> NotificationMessageV2::removedFlags() const
{
  return d->removedFlags;
}

void NotificationMessageV2::setRemovedFlags( const QSet<QByteArray> &removedFlags )
{
  d->removedFlags = removedFlags;
}

QString NotificationMessageV2::toString() const
{
  QString rv;

  switch ( d->type ) {
    case Items:
      rv += QLatin1String( "Items " );
      break;
    case Collections:
      rv += QLatin1String( "Collections " );
      break;
    case InvalidType:
      return QLatin1String( "*INVALID TYPE* " );
  }

  QSet<QByteArray> items;
  Q_FOREACH ( const NotificationMessageV2::Item &item, d->items ) {
    QString itemStr = QString::fromLatin1( "(%1,%2" ).arg( item.id ).arg( item.remoteId );
    if ( !item.remoteRevision.isEmpty() ) {
      itemStr += QString::fromLatin1( ",%1" ).arg( item.remoteRevision );
    }
    if ( !item.mimeType.isEmpty() ) {
      itemStr += QString::fromLatin1( ",%1" ).arg( item.mimeType );
    }
    itemStr += QLatin1String( ")" );
    items << itemStr.toLatin1();
  }
  rv += QLatin1String( "(" ) + QString::fromLatin1( ImapParser::join( items, ", " ) ) + QLatin1String( ")" );

  if ( d->parentDestCollection >= 0 ) {
    rv += QLatin1String( " from " );
  } else {
    rv += QLatin1String( " in " );
  }

  if ( d->parentCollection >= 0 ) {
    rv += QString::fromLatin1( "collection %1 " ).arg( d->parentCollection );
  } else {
    rv += QLatin1String( "unspecified parent collection " );
  }

  switch ( d->operation ) {
    case Add:
      rv += QLatin1String( "added" );
      break;
    case Modify:
      rv += QLatin1String( "modified parts (" );
      rv += QString::fromLatin1( ImapParser::join( d->parts.toList(), ", " ) );
      rv += QLatin1String( ")" );
      break;
    case ModifyFlags:
      rv += QLatin1String( "added flags (" );
      rv += QString::fromLatin1( ImapParser::join( d->addedFlags.toList(), ", " ) );
      rv += QLatin1String ( ") " );

      rv += QLatin1String( "removed flags (" );
      rv += QString::fromLatin1( ImapParser::join( d->removedFlags.toList(), ", " ) );
      rv += QLatin1String ( ") " );
      break;
    case Move:
      rv += QLatin1String( "moved" );
      break;
    case Remove:
      rv += QLatin1String( "removed" );
      break;
    case Link:
      rv += QLatin1String( "linked" );
      break;
    case Unlink:
      rv += QLatin1String( "unlinked" );
      break;
    case Subscribe:
      rv += QLatin1String( "subscribed" );
      break;
    case Unsubscribe:
      rv += QLatin1String( "unsubscribed" );
      break;
    case InvalidOp:
      return QLatin1String( "*INVALID OPERATION*" );
  }

  if ( d->parentDestCollection >= 0 )
    rv += QString::fromLatin1( " to collection %1" ).arg( d->parentDestCollection );

  return rv;
}

QDBusArgument& operator<<( QDBusArgument &arg, const Akonadi::NotificationMessageV2 &msg )
{
  arg.beginStructure();
  arg << msg.sessionId();
  arg << static_cast<int>( msg.type() );
  arg << static_cast<int>( msg.operation() );
  arg << msg.entities().values();
  arg << msg.resource();
  arg << msg.destinationResource();
  arg << msg.parentCollection();
  arg << msg.parentDestCollection();

  QStringList itemParts;
  Q_FOREACH ( const QByteArray &itemPart, msg.itemParts() ) {
    itemParts.append( QString::fromLatin1( itemPart ) );
  }
  arg << itemParts;

  arg << msg.addedFlags().toList();
  arg << msg.removedFlags().toList();

  arg.endStructure();
  return arg;
}

const QDBusArgument& operator>>( const QDBusArgument &arg, Akonadi::NotificationMessageV2 &msg )
{
  QByteArray ba;
  int i;
  QList<NotificationMessageV2::Item> items;
  NotificationMessageV2::Id id;
  QString str;
  QStringList strl;
  QList<QByteArray> bal;

  arg.beginStructure();
  arg >> ba;
  msg.setSessionId( ba );
  arg >> i;
  msg.setType( static_cast<NotificationMessageV2::Type>( i ) );
  arg >> i;
  msg.setOperation( static_cast<NotificationMessageV2::Operation>( i ) );
  arg >> items;
  msg.setEntities( items );
  arg >> ba;
  msg.setResource( ba );
  arg >> ba;
  msg.setDestinationResource( ba );
  arg >> id;
  msg.setParentCollection( id );
  arg >> id;
  msg.setParentDestCollection( id );

  arg >> strl;

  QSet<QByteArray> itemParts;
  Q_FOREACH ( const QString &itemPart, strl ) {
    itemParts.insert( itemPart.toLatin1() );
  }
  msg.setItemParts( itemParts );

  arg >> bal;
  msg.setAddedFlags( bal.toSet() );
  arg >> bal;
  msg.setRemovedFlags( bal.toSet() );

  arg.endStructure();
  return arg;
}

QDBusArgument& operator<<( QDBusArgument &arg, const Akonadi::NotificationMessageV2::Item &item )
{
  arg.beginStructure();
  arg << item.id;
  arg << item.remoteId;
  arg << item.remoteRevision;
  arg << item.mimeType;
  arg.endStructure();

  return arg;
}

const QDBusArgument& operator>>( const QDBusArgument &arg, Akonadi::NotificationMessageV2::Item &item )
{
  arg.beginStructure();
  arg >> item.id;
  arg >> item.remoteId;
  arg >> item.remoteRevision;
  arg >> item.mimeType;
  arg.endStructure();

  return arg;
}

uint qHash( const Akonadi::NotificationMessageV2 &msg )
{
  uint i = 0;
  Q_FOREACH( const NotificationMessageV2::Item &item, msg.entities() ) {
    i += item.id;
  }

  return qHash( i + (msg.type() << 31) + (msg.operation() << 28) );
}

QVector<NotificationMessage> NotificationMessageV2::toNotificationV1() const
{
  QVector<NotificationMessage> v1;

  Q_FOREACH( const Item &item, d->items ) {
    NotificationMessage msgv1;
    msgv1.setSessionId( d->sessionId );
    msgv1.setUid( item.id );
    msgv1.setRemoteId( item.remoteId );
    msgv1.setMimeType( item.mimeType );
    msgv1.setType( static_cast<NotificationMessage::Type>( d->type ) );
    if ( d->operation == ModifyFlags ) {
      msgv1.setOperation( NotificationMessage::Modify );
    } else {
      msgv1.setOperation( static_cast<NotificationMessage::Operation>( d->operation ) );
    }

    msgv1.setResource( d->resource );
    msgv1.setDestinationResource( d->destResource );
    msgv1.setParentCollection( d->parentCollection );
    msgv1.setParentDestCollection( d->parentDestCollection );

    // Backward compatibility hack
    QSet<QByteArray> parts;
    if ( d->operation == Remove ) {
      QByteArray rr = item.remoteRevision.toLatin1();
      parts << (rr.isEmpty() ? "1" : rr);
    } else if ( d->operation == ModifyFlags ) {
      parts << "FLAGS";
    } else {
      parts = d->parts;
    }
    msgv1.setItemParts( parts );

    v1 << msgv1;
  }

  return v1;
}

bool NotificationMessageV2::appendAndCompress( NotificationMessageV2::List &list, const NotificationMessageV2 &msg )
{
  // fast-path for stuff that is not considered during O(n) compression below
  if ( msg.operation() != Add && msg.operation() != Link && msg.operation() != Unlink && msg.operation() != Subscribe && msg.operation() != Unsubscribe && msg.operation() != Move ) {

    NotificationMessageV2::List::Iterator end = list.end();
    for ( NotificationMessageV2::List::Iterator it = list.begin(); it != end; ) {
      if ( msg.d.constData()->compareWithoutOpAndParts( *((*it).d.constData()) ) ) {

        // both are modifications, merge them together and drop the new one
        if (( msg.operation() == Modify || msg.operation() == ModifyFlags ) && ( it->operation() == Modify || it->operation() == ModifyFlags )) {
            (*it).setItemParts( (*it).itemParts() + msg.itemParts() );
            (*it).setAddedFlags( (*it).addedFlags() + msg.addedFlags() );
            (*it).setRemovedFlags( (*it).removedFlags() + msg.removedFlags() );

            // If merged notifications result in no-change notification, drop both.
            if ( it->operation() == ModifyFlags ) {
              if ( (*it).addedFlags() == (*it).removedFlags() ) {
                it = list.erase( it );
                end = list.end();
              }
            }

          return false;
        }
        // new one is a modification, the existing one not, so drop the new one
        else if (( msg.operation() == Modify ) || ( msg.operation() == ModifyFlags )) {
          return false;
        }
        // new on is a deletion, erase the existing modification ones (and keep going, in case there are more)
        else if ( msg.operation() == Remove && ((*it).operation() == Modify || (*it).operation() == ModifyFlags) ) {
          it = list.erase( it );
          end = list.end();
        }
        // keep looking
        else {
          ++it;
        }
      } else {
        ++it;
      }
    }
  }

  list.append( msg );
  return true;
}
