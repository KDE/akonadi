/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "protocolhelper_p.h"

#include "attributefactory.h"
#include "collectionstatistics.h"
#include "entity_p.h"
#include "exception.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QVarLengthArray>

#include <kdebug.h>
#include <klocale.h>

using namespace Akonadi;

int ProtocolHelper::parseCachePolicy(const QByteArray & data, CachePolicy & policy, int start)
{
  QVarLengthArray<QByteArray,16> params;
  int end = Akonadi::ImapParser::parseParenthesizedList( data, params, start );
  for ( int i = 0; i < params.count() - 1; i += 2 ) {
    const QByteArray key = params[i];
    const QByteArray value = params[i + 1];

    if ( key == "INHERIT" )
      policy.setInheritFromParent( value == "true" );
    else if ( key == "INTERVAL" )
      policy.setIntervalCheckTime( value.toInt() );
    else if ( key == "CACHETIMEOUT" )
      policy.setCacheTimeout( value.toInt() );
    else if ( key == "SYNCONDEMAND" )
      policy.setSyncOnDemand( value == "true" );
    else if ( key == "LOCALPARTS" ) {
      QVarLengthArray<QByteArray,16> tmp;
      QStringList parts;
      Akonadi::ImapParser::parseParenthesizedList( value, tmp );
      for ( int j=0; j<tmp.size(); j++ )
        parts << QString::fromLatin1( tmp[j] );
      policy.setLocalParts( parts );
    }
  }
  return end;
}

QByteArray ProtocolHelper::cachePolicyToByteArray(const CachePolicy & policy)
{
  QByteArray rv = "CACHEPOLICY (";
  if ( policy.inheritFromParent() ) {
    rv += "INHERIT true";
  } else {
    rv += "INHERIT false";
    rv += " INTERVAL " + QByteArray::number( policy.intervalCheckTime() );
    rv += " CACHETIMEOUT " + QByteArray::number( policy.cacheTimeout() );
    rv += " SYNCONDEMAND " + ( policy.syncOnDemand() ? QByteArray("true") : QByteArray("false") );
    rv += " LOCALPARTS (" + policy.localParts().join( QLatin1String(" ") ).toLatin1() + ')';
  }
  rv += ')';
  return rv;
}

void ProtocolHelper::parseAncestors( const QByteArray &data, Entity *entity, int start )
{
  Q_UNUSED( start );

  QList<QByteArray> ancestors;
  ImapParser::parseParenthesizedList( data, ancestors );
  Entity* current = entity;
  foreach ( const QByteArray &uidRidPair, ancestors ) {
    QList<QByteArray> parentIds;
    ImapParser::parseParenthesizedList( uidRidPair, parentIds );
    if ( parentIds.size() != 2 )
      break;
    const Collection::Id uid = parentIds.at( 0 ).toLongLong();
    const QString rid = QString::fromUtf8( parentIds.at( 1 ) );
    if ( uid == Collection::root().id() ) {
      current->setParentCollection( Collection::root() );
      break;
    }
    current->parentCollection().setId( uid );
    current->parentCollection().setRemoteId( rid );
    current = &current->parentCollection();
  }
}

int ProtocolHelper::parseCollection(const QByteArray & data, Collection & collection, int start)
{
  int pos = start;

  // collection and parent id
  Collection::Id colId = -1;
  bool ok = false;
  pos = ImapParser::parseNumber( data, colId, &ok, pos );
  if ( !ok || colId <= 0 ) {
    kDebug() << "Could not parse collection id from response:" << data;
    return start;
  }

  Collection::Id parentId = -1;
  pos = ImapParser::parseNumber( data, parentId, &ok, pos );
  if ( !ok || parentId < 0 ) {
    kDebug() << "Could not parse parent id from response:" << data;
    return start;
  }

  collection = Collection( colId );
  collection.setParentCollection( Collection( parentId ) );

  // attributes
  QVarLengthArray<QByteArray,16> attributes;
  pos = ImapParser::parseParenthesizedList( data, attributes, pos );

  for ( int i = 0; i < attributes.count() - 1; i += 2 ) {
    const QByteArray key = attributes[i];
    const QByteArray value = attributes[i + 1];

    if ( key == "NAME" ) {
      collection.setName( QString::fromUtf8( value ) );
    } else if ( key == "REMOTEID" ) {
      collection.setRemoteId( QString::fromUtf8( value ) );
    } else if ( key == "RESOURCE" ) {
      collection.setResource( QString::fromUtf8( value ) );
    } else if ( key == "MIMETYPE" ) {
      QVarLengthArray<QByteArray,16> ct;
      ImapParser::parseParenthesizedList( value, ct );
      QStringList ct2;
      for ( int j = 0; j < ct.size(); j++ )
        ct2 << QString::fromLatin1( ct[j] );
      collection.setContentMimeTypes( ct2 );
    } else if ( key == "MESSAGES" ) {
      CollectionStatistics s = collection.statistics();
      s.setCount( value.toLongLong() );
      collection.setStatistics( s );
    } else if ( key == "UNSEEN" ) {
      CollectionStatistics s = collection.statistics();
      s.setUnreadCount( value.toLongLong() );
      collection.setStatistics( s );
    } else if ( key == "SIZE" ) {
      CollectionStatistics s = collection.statistics();
      s.setSize( value.toLongLong() );
      collection.setStatistics( s );
    } else if ( key == "CACHEPOLICY" ) {
      CachePolicy policy;
      ProtocolHelper::parseCachePolicy( value, policy );
      collection.setCachePolicy( policy );
    } else if ( key == "ANCESTORS" ) {
      parseAncestors( value, &collection );
    } else {
      Attribute* attr = AttributeFactory::createAttribute( key );
      Q_ASSERT( attr );
      attr->deserialize( value );
      collection.addAttribute( attr );
    }
  }

  return pos;
}

QByteArray ProtocolHelper::attributesToByteArray(const Entity & entity, bool ns )
{
  QList<QByteArray> l;
  foreach ( const Attribute *attr, entity.attributes() ) {
    l << encodePartIdentifier( ns ? PartAttribute : PartGlobal, attr->type() );
    l << ImapParser::quote( attr->serialized() );
  }
  return ImapParser::join( l, " " );
}

QByteArray ProtocolHelper::encodePartIdentifier(PartNamespace ns, const QByteArray & label, int version )
{
  const QByteArray versionString( version != 0 ? '[' + QByteArray::number( version ) + ']' : "" );
  switch ( ns ) {
    case PartGlobal:
      return label + versionString;
    case PartPayload:
      return "PLD:" + label + versionString;
    case PartAttribute:
      return "ATR:" + label + versionString;
    default:
      Q_ASSERT( false );
  }
  return QByteArray();
}

QByteArray ProtocolHelper::decodePartIdentifier( const QByteArray &data, PartNamespace & ns )
{
  if ( data.startsWith( "PLD:" ) ) { //krazy:exclude=strings
    ns = PartPayload;
    return data.mid( 4 );
  } else if ( data.startsWith( "ATR:" ) ) { //krazy:exclude=strings
    ns = PartAttribute;
    return data.mid( 4 );
  } else {
    ns = PartGlobal;
    return data;
  }
}

QByteArray ProtocolHelper::hierarchicalRidToByteArray( const Collection &col )
{
  if ( col == Collection::root() )
    return QByteArray("(0 \"\")");
  if ( col.remoteId().isEmpty() )
    return QByteArray();
  const QByteArray parentHrid = hierarchicalRidToByteArray( col.parentCollection() );
  return '(' + QByteArray::number( col.id() ) + ' ' + ImapParser::quote( col.remoteId().toUtf8() ) + ") " + parentHrid;
}

QByteArray ProtocolHelper::hierarchicalRidToByteArray( const Item &item )
{
  const QByteArray parentHrid = hierarchicalRidToByteArray( item.parentCollection() );
  return '(' + QByteArray::number( item.id() ) + ' ' + ImapParser::quote( item.remoteId().toUtf8() ) + ") " + parentHrid;
}  

QByteArray ProtocolHelper::itemFetchScopeToByteArray( const ItemFetchScope &fetchScope )
{
  QByteArray command;

  if ( fetchScope.fullPayload() )
    command += " " AKONADI_PARAM_FULLPAYLOAD;
  if ( fetchScope.allAttributes() )
    command += " " AKONADI_PARAM_ALLATTRIBUTES;
  if ( fetchScope.cacheOnly() )
    command += " " AKONADI_PARAM_CACHEONLY;
  if ( fetchScope.ancestorRetrieval() != ItemFetchScope::None ) {
    switch ( fetchScope.ancestorRetrieval() ) {
      case ItemFetchScope::Parent:
        command += " ANCESTORS 1";
        break;
      case ItemFetchScope::All:
        command += " ANCESTORS INF";
        break;
      default:
        Q_ASSERT( false );
    }
  }

  //TODO: detect somehow if server supports external payload attribute
  command += " " AKONADI_PARAM_EXTERNALPAYLOAD;

  command += " (UID REMOTEID COLLECTIONID FLAGS SIZE DATETIME";
  foreach ( const QByteArray &part, fetchScope.payloadParts() )
    command += ' ' + ProtocolHelper::encodePartIdentifier( ProtocolHelper::PartPayload, part );
  foreach ( const QByteArray &part, fetchScope.attributes() )
    command += ' ' + ProtocolHelper::encodePartIdentifier( ProtocolHelper::PartAttribute, part );
  command += ")\n";

  return command;
}

void ProtocolHelper::parseItemFetchResult( const QList<QByteArray> &lineTokens, Item &item )
{
  // create a new item object
  Item::Id uid = -1;
  int rev = -1;
  QString rid;
  QString mimeType;
  Entity::Id cid = -1;

  for ( int i = 0; i < lineTokens.count() - 1; i += 2 ) {
    const QByteArray key = lineTokens.value( i );
    const QByteArray value = lineTokens.value( i + 1 );

    if ( key == "UID" )
      uid = value.toLongLong();
    else if ( key == "REV" )
      rev = value.toInt();
    else if ( key == "REMOTEID" ) {
      if ( !value.isEmpty() )
        rid = QString::fromUtf8( value );
      else
        rid.clear();
    } else if ( key == "COLLECTIONID" ) {
      cid = value.toInt();
    } else if ( key == "MIMETYPE" )
      mimeType = QString::fromLatin1( value );
  }

  if ( uid < 0 || rev < 0 || mimeType.isEmpty() ) {
    kWarning() << "Broken fetch response: UID, RID, REV or MIMETYPE missing!";
    return;
  }

  item = Item( uid );
  item.setRemoteId( rid );
  item.setRevision( rev );
  item.setMimeType( mimeType );
  item.setStorageCollectionId( cid );
  if ( !item.isValid() )
    return;

  // parse fetch response fields
  for ( int i = 0; i < lineTokens.count() - 1; i += 2 ) {
    const QByteArray key = lineTokens.value( i );
    // skip stuff we dealt with already
    if ( key == "UID" || key == "REV" || key == "REMOTEID" || key == "MIMETYPE"  || key == "COLLECTIONID")
      continue;
    // flags
    if ( key == "FLAGS" ) {
      QList<QByteArray> flags;
      ImapParser::parseParenthesizedList( lineTokens[i + 1], flags );
      foreach ( const QByteArray &flag, flags ) {
        item.setFlag( flag );
      }
    } else if ( key == "SIZE" ) {
      const quint64 size = lineTokens[i + 1].toLongLong();
      item.setSize( size );
    } else if ( key == "DATETIME" ) {
      QDateTime datetime;
      ImapParser::parseDateTime( lineTokens[i + 1], datetime );
      item.setModificationTime( datetime );
    } else if ( key == "ANCESTORS" ) {
      ProtocolHelper::parseAncestors( lineTokens[i + 1], &item );
    } else {
      int version = 0;
      QByteArray plainKey( key );
      ProtocolHelper::PartNamespace ns;

      ImapParser::splitVersionedKey( key, plainKey, version );
      plainKey = ProtocolHelper::decodePartIdentifier( plainKey, ns );

      switch ( ns ) {
        case ProtocolHelper::PartPayload:
        {
          bool isExternal = false;
          const QByteArray fileKey = lineTokens.value( i + 1 );
          if ( fileKey == "[FILE]" ) {
            isExternal = true;
            i++;
            kDebug() << "Payload is external: " << isExternal << " filename: " << lineTokens.value( i + 1 );
          }
          ItemSerializer::deserialize( item, plainKey, lineTokens.value( i + 1 ), version, isExternal );
          break;
        }
        case ProtocolHelper::PartAttribute:
        {
          Attribute* attr = AttributeFactory::createAttribute( plainKey );
          Q_ASSERT( attr );
          if ( lineTokens.value( i + 1 ) == "[FILE]" ) {
            ++i;
            QFile file( QString::fromUtf8( lineTokens.value( i + 1 ) ) );
            if ( file.open( QFile::ReadOnly ) )
              attr->deserialize( file.readAll() );
            else {
              kWarning() << "Failed to open attribute file: " << lineTokens.value( i + 1 );
              delete attr;
            }
          } else {
            attr->deserialize( lineTokens.value( i + 1 ) );
          }
          item.addAttribute( attr );
          break;
        }
        case ProtocolHelper::PartGlobal:
        default:
          kWarning() << "Unknown item part type:" << key;
      }
    }
  }

  item.d_ptr->resetChangeLog();
}
