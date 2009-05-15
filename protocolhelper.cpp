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
#include "exception.h"
#include <akonadi/private/imapparser_p.h>
#include <akonadi/private/protocol_p.h>

#include <QtCore/QVarLengthArray>

#include <kdebug.h>
#include <klocale.h>

#include <boost/bind.hpp>
#include <algorithm>

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

int ProtocolHelper::parseCollection(const QByteArray & data, Collection & collection, int start)
{
  int pos = start;

  // collection and parent id
  Collection::Id colId = -1;
  bool ok = false;
  pos = ImapParser::parseNumber( data, colId, &ok, pos );
  if ( !ok || colId <= 0 ) {
    kDebug( 5250 ) << "Could not parse collection id from response:" << data;
    return start;
  }

  Collection::Id parentId = -1;
  pos = ImapParser::parseNumber( data, parentId, &ok, pos );
  if ( !ok || parentId < 0 ) {
    kDebug( 5250 ) << "Could not parse parent id from response:" << data;
    return start;
  }

  collection = Collection( colId );
  collection.setParent( parentId );

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
    } else if ( key == "CACHEPOLICY" ) {
      CachePolicy policy;
      ProtocolHelper::parseCachePolicy( value, policy );
      collection.setCachePolicy( policy );
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

QByteArray ProtocolHelper::itemSetToByteArray( const Item::List &_items, const QByteArray &command )
{
  if ( _items.isEmpty() )
    throw Exception( "No items specified" );

  Item::List items( _items );

  QByteArray rv;
  std::sort( items.begin(), items.end(), boost::bind( &Item::id, _1 ) < boost::bind( &Item::id, _2 ) );
  if ( items.first().isValid() ) {
    // all items have a uid set
    rv += " " AKONADI_CMD_UID " ";
    rv += command;
    rv += ' ';
    QList<Item::Id>  uids;
    foreach ( const Item &item, items )
      uids << item.id();
    ImapSet set;
    set.add( uids );
    rv += set.toImapSequenceSet();
  } else {
    // check if all items have a remote id
    QList<QByteArray> rids;
    foreach ( const Item &item, items ) {
      if ( item.remoteId().isEmpty() )
        throw Exception( i18n( "No remote identifier specified" ) );
      rids << ImapParser::quote( item.remoteId().toUtf8() );
    }

    rv += " " AKONADI_CMD_RID " ";
    rv += command;
    rv += " (";
    rv += ImapParser::join( rids, " " );
    rv += ')';
  }
  return rv;
}
