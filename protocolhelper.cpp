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
#include "servermanager.h"
#include "tagfetchscope.h"
#include <akonadi/private/xdgbasedirs_p.h>

#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QVarLengthArray>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <kdebug.h>
#include <klocalizedstring.h>

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

void ProtocolHelper::parseAncestorsCached( const QByteArray &data, Entity *entity, Collection::Id parentCollection,
                                           ProtocolHelperValuePool *pool, int start )
{
  if ( !pool || parentCollection == -1 ) {
    // if no pool or parent collection id is provided we can't cache anything, so continue as usual
    parseAncestors( data, entity, start );
    return;
  }

  if ( pool->ancestorCollections.contains( parentCollection ) ) {
    // ancestor chain is cached already, so use the cached value
    entity->setParentCollection( pool->ancestorCollections.value( parentCollection ) );
  } else {
    // not cached yet, parse the chain
    parseAncestors( data, entity, start );
    pool->ancestorCollections.insert( parentCollection, entity->parentCollection() );
  }
}

void ProtocolHelper::parseAncestors( const QByteArray &data, Entity *entity, int start )
{
  Q_UNUSED( start );

  static const Collection::Id rootCollectionId = Collection::root().id();
  QVarLengthArray<QByteArray, 16> ancestors;
  // QVarLengthArray<QByteArray, 16> parentIds;
  QList<QByteArray> parentIds;

  ImapParser::parseParenthesizedList( data, ancestors );
  Entity* current = entity;
  for ( int i = 0; i < ancestors.count(); ++i ) {
    parentIds.clear();
    ImapParser::parseParenthesizedList( ancestors[ i ], parentIds );
    if ( parentIds.size() < 2 )
      break;

    const Collection::Id uid = parentIds[ 0 ].toLongLong();
    if ( uid == rootCollectionId ) {
      current->setParentCollection( Collection::root() );
      break;
    }

    Akonadi::Collection parentCollection;
    parseCollection( ImapParser::join(parentIds, " "), parentCollection, 0, false );
    current->setParentCollection(parentCollection);

    current = &current->parentCollection();
  }
}

static Collection::ListPreference parsePreference( const QByteArray &value )
{
  if ( value == "TRUE" ) {
    return Collection::ListEnabled;
  }
  if ( value == "FALSE" ) {
    return Collection::ListDisabled;
  }
  return Collection::ListDefault;
}

int ProtocolHelper::parseCollection(const QByteArray & data, Collection & collection, int start, bool requireParent)
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

  collection = Collection( colId );

  if (requireParent) {
    Collection::Id parentId = -1;
    pos = ImapParser::parseNumber( data, parentId, &ok, pos );
    if ( !ok || parentId < 0 ) {
        kDebug() << "Could not parse parent id from response:" << data;
        return start;
    }
    collection.setParentCollection( Collection( parentId ) );
  }

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
    } else if ( key == "REMOTEREVISION" ) {
      collection.setRemoteRevision( QString::fromUtf8( value ) );
    } else if ( key == "RESOURCE" ) {
      collection.setResource( QString::fromUtf8( value ) );
    } else if ( key == "MIMETYPE" ) {
      QVarLengthArray<QByteArray,16> ct;
      ImapParser::parseParenthesizedList( value, ct );
      QStringList ct2;
      for ( int j = 0; j < ct.size(); j++ )
        ct2 << QString::fromLatin1( ct[j] );
      collection.setContentMimeTypes( ct2 );
    } else if ( key == "VIRTUAL" ) {
      collection.setVirtual( value.toUInt() != 0 );
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
    } else if ( key == "ENABLED" ) {
      collection.setEnabled( value == "TRUE" );
    } else if ( key == "DISPLAY" ) {
      collection.setLocalListPreference( Collection::ListDisplay, parsePreference( value ) );
    } else if ( key == "SYNC" ) {
      collection.setLocalListPreference( Collection::ListSync, parsePreference( value ) );
    } else if ( key == "INDEX" ) {
      collection.setLocalListPreference( Collection::ListIndex, parsePreference( value ) );
    } else if ( key == "REFERENCED" ) {
      collection.setReferenced( value == "TRUE" );
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

QByteArray ProtocolHelper::attributesToByteArray(const AttributeEntity & entity, bool ns )
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
  const QByteArray versionString( version != 0 ? QByteArray(QByteArray("[") + QByteArray::number( version ) + QByteArray("]")) : "" );
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

QByteArray ProtocolHelper::entitySetToByteArray( const QList<Item> &_objects, const QByteArray &command )
{
  if ( _objects.isEmpty() )
    throw Exception( "No objects specified" );

  Item::List objects( _objects );
  std::sort( objects.begin(), objects.end(), boost::bind( &Item::id, _1 ) < boost::bind( &Item::id, _2 ) );
  if ( objects.first().isValid() ) {
    // all items have a uid set
    return entitySetToByteArray<Item>(objects, command);
  }
  // check if all items have a gid
  if ( std::find_if( objects.constBegin(), objects.constEnd(),
    boost::bind( &QString::isEmpty, boost::bind( &Item::gid, _1 ) ) )
    == objects.constEnd() )
  {
    QList<QByteArray> gids;
    foreach ( const Item &object, objects ) {
        gids << ImapParser::quote( object.gid().toUtf8() );
    }

    QByteArray rv;
    //rv += " " AKONADI_CMD_GID " ";
    rv += " " "GID" " ";
    if ( !command.isEmpty() ) {
        rv += command;
        rv += ' ';
    }
    rv += '(';
    rv += ImapParser::join( gids, " " );
    rv += ')';
    return rv;
  }
  return entitySetToByteArray<Item>(objects, command);
}

QByteArray ProtocolHelper::tagSetToImapSequenceSet( const Akonadi::Tag::List &_objects )
{
  if ( _objects.isEmpty() )
    throw Exception( "No objects specified" );

  Tag::List objects( _objects );

  std::sort( objects.begin(), objects.end(), boost::bind( &Tag::id, _1 ) < boost::bind( &Tag::id, _2 ) );
  if ( !objects.first().isValid() ) {
    throw Exception( "Not all tags have a uid" );
  }
  // all items have a uid set
  QVector<Tag::Id>  uids;
  foreach ( const Tag &object, objects )
    uids << object.id();
  ImapSet set;
  set.add( uids );
  return set.toImapSequenceSet();
}

QByteArray ProtocolHelper::tagSetToByteArray( const Tag::List &_objects, const QByteArray &command )
{
  if ( _objects.isEmpty() )
    throw Exception( "No objects specified" );

  Tag::List objects( _objects );

  QByteArray rv;
  std::sort( objects.begin(), objects.end(), boost::bind( &Tag::id, _1 ) < boost::bind( &Tag::id, _2 ) );
  if ( objects.first().isValid() ) {
    // all items have a uid set
    rv += " " AKONADI_CMD_UID " ";
    if ( !command.isEmpty() ) {
      rv += command;
      rv += ' ';
    }
    QVector<Tag::Id>  uids;
    foreach ( const Tag &object, objects )
      uids << object.id();
    ImapSet set;
    set.add( uids );
    rv += set.toImapSequenceSet();
    return rv;
  }
  throw Exception( "Not all tags have a uid" );
}

QByteArray ProtocolHelper::commandContextToByteArray(const Akonadi::Collection &collection, const Akonadi::Tag &tag,
                                                     const Item::List &requestedItems, const QByteArray &command)
{
    QByteArray r = " ";
    if (requestedItems.isEmpty()) {
        r += command + " 1:*";
    } else {
        r += ProtocolHelper::entitySetToByteArray(requestedItems, command);
    }

    if (tag.isValid()) {
        r += " " AKONADI_PARAM_TAGID " " + QByteArray::number(tag.id()) + " ";
    }

    if (collection == Collection::root()) {
        if (requestedItems.isEmpty()) {   // collection content listing
            throw Exception("Cannot perform item operations on root collection.");
        }
    } else {
        if (collection.isValid()) {
            r += " " AKONADI_PARAM_COLLECTIONID " " + QByteArray::number(collection.id()) + ' ';
        } else if (!collection.remoteId().isEmpty()) {
            r += " " AKONADI_PARAM_COLLECTION " " + ImapParser::quote(collection.remoteId().toUtf8()) + ' ';
        }
    }

    return r;
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
  if ( fetchScope.checkForCachedPayloadPartsOnly() )
    command += " " AKONADI_PARAM_CHECKCACHEDPARTSONLY;
  if ( fetchScope.ignoreRetrievalErrors() )
    command += " " "IGNOREERRORS";
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
  if ( fetchScope.fetchChangedSince().isValid() ) {
    command += " " AKONADI_PARAM_CHANGEDSINCE " " + QByteArray::number( fetchScope.fetchChangedSince().toTime_t() );
  }

  //TODO: detect somehow if server supports external payload attribute
  command += " " AKONADI_PARAM_EXTERNALPAYLOAD;

  command += " (UID COLLECTIONID FLAGS SIZE";
  if ( fetchScope.fetchRemoteIdentification() )
    command += " " AKONADI_PARAM_REMOTEID " " AKONADI_PARAM_REMOTEREVISION;
  if ( fetchScope.fetchGid() )
    command += " GID";
  if ( fetchScope.fetchTags() ) {
    command += " TAGS";
    if ( !fetchScope.tagFetchScope().fetchIdOnly() ) {
      command += " " + ProtocolHelper::tagFetchScopeToByteArray( fetchScope.tagFetchScope() );
    }
  }
  if ( fetchScope.fetchVirtualReferences() )
    command += " VIRTREF";
  if ( fetchScope.fetchModificationTime() )
    command += " DATETIME";
  foreach ( const QByteArray &part, fetchScope.payloadParts() )
    command += ' ' + ProtocolHelper::encodePartIdentifier( ProtocolHelper::PartPayload, part );
  foreach ( const QByteArray &part, fetchScope.attributes() )
    command += ' ' + ProtocolHelper::encodePartIdentifier( ProtocolHelper::PartAttribute, part );
  command += ")\n";

  return command;
}

QByteArray ProtocolHelper::tagFetchScopeToByteArray( const TagFetchScope &fetchScope )
{
  QByteArray command;

  command += "(UID";
  Q_FOREACH (const QByteArray &part, fetchScope.attributes()) {
    command += ' ' + ProtocolHelper::encodePartIdentifier(ProtocolHelper::PartAttribute, part);
  }
  command += ")";
  return command;
}

void ProtocolHelper::parseItemFetchResult( const QList<QByteArray> &lineTokens, Item &item, ProtocolHelperValuePool *valuePool )
{
  // create a new item object
  Item::Id uid = -1;
  int rev = -1;
  QString rid;
  QString remoteRevision;
  QString gid;
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
    } else if ( key == "REMOTEREVISION" ) {
      remoteRevision = QString::fromUtf8( value );
    } else if ( key == "GID" ) {
      gid = QString::fromUtf8( value );
    } else if ( key == "COLLECTIONID" ) {
      cid = value.toInt();
    } else if ( key == "MIMETYPE" ) {
      if ( valuePool )
        mimeType = valuePool->mimeTypePool.sharedValue( QString::fromLatin1( value ) );
      else
        mimeType = QString::fromLatin1( value );
    }
  }

  if ( uid < 0 || rev < 0 || mimeType.isEmpty() ) {
    kWarning() << "Broken fetch response: UID, REV or MIMETYPE missing!";
    return;
  }

  item = Item( uid );
  item.setRemoteId( rid );
  item.setRevision( rev );
  item.setRemoteRevision( remoteRevision );
  item.setGid( gid );
  item.setMimeType( mimeType );
  item.setStorageCollectionId( cid );
  if ( !item.isValid() )
    return;

  // parse fetch response fields
  for ( int i = 0; i < lineTokens.count() - 1; i += 2 ) {
    const QByteArray key = lineTokens.value( i );
    // skip stuff we dealt with already
    if ( key == "UID" || key == "REV" || key == "REMOTEID" ||
         key == "MIMETYPE"  || key == "COLLECTIONID" || key == "REMOTEREVISION" || key == "GID" )
      continue;
    // flags
    if ( key == "FLAGS" ) {
      QList<QByteArray> flags;
      ImapParser::parseParenthesizedList( lineTokens[i + 1], flags );
      if ( !flags.isEmpty() ) {
        Item::Flags convertedFlags;
        convertedFlags.reserve( flags.size() );
        foreach ( const QByteArray &flag, flags ) {
          if ( valuePool )
            convertedFlags.insert( valuePool->flagPool.sharedValue( flag ) );
          else
            convertedFlags.insert( flag );
        }
        item.setFlags( convertedFlags );
      }
    } else if ( key == "TAGS" ) {
      Tag::List tags;
      if ( lineTokens[i + 1].startsWith("(") ) {
        QList<QByteArray> tagsData;
        ImapParser::parseParenthesizedList( lineTokens[i + 1], tagsData );
        Q_FOREACH (const QByteArray &t, tagsData) {
          QList<QByteArray> tagParts;
          ImapParser::parseParenthesizedList( t, tagParts );
          Tag tag;
          parseTagFetchResult(tagParts, tag);
          tags << tag;
        }
      } else {
        ImapSet set;
        ImapParser::parseSequenceSet( lineTokens[i + 1], set );
        Q_FOREACH ( const ImapInterval &interval, set.intervals() ) {
          Q_ASSERT( interval.hasDefinedBegin() );
          Q_ASSERT( interval.hasDefinedEnd() );
          for ( qint64 i = interval.begin(); i <= interval.end(); i++ ) {
            //TODO use value pool when tag is shared data
            tags << Tag( i );
          }
        }
      }
      item.setTags( tags );
    } else if ( key == "VIRTREF" ) {
      ImapSet set;
      ImapParser::parseSequenceSet( lineTokens[i + 1], set );
      Collection::List collections;
      Q_FOREACH ( const ImapInterval &interval, set.intervals() ) {
        Q_ASSERT( interval.hasDefinedBegin() );
        Q_ASSERT( interval.hasDefinedEnd() );
        for ( qint64 i = interval.begin(); i <= interval.end(); i++ ) {
          collections << Collection(i);
        }
      }
      item.setVirtualReferences(collections);
    } else if ( key == "CACHEDPARTS" ) {
      QSet<QByteArray> partsSet;
      QList<QByteArray> parts;
      ImapParser::parseParenthesizedList( lineTokens[i + 1], parts );
      foreach ( const QByteArray &part, parts ) {
        partsSet.insert(part.mid(4));
      }
      item.setCachedPayloadParts( partsSet );
    } else if ( key == "SIZE" ) {
      const quint64 size = lineTokens[i + 1].toLongLong();
      item.setSize( size );
    } else if ( key == "DATETIME" ) {
      QDateTime datetime;
      ImapParser::parseDateTime( lineTokens[i + 1], datetime );
      item.setModificationTime( datetime );
    } else if ( key == "ANCESTORS" ) {
      ProtocolHelper::parseAncestorsCached( lineTokens[i + 1], &item, cid, valuePool );
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
            //kDebug() << "Payload is external: " << isExternal << " filename: " << lineTokens.value( i + 1 );
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
              attr = 0;
            }
          } else {
            attr->deserialize( lineTokens.value( i + 1 ) );
          }
          if ( attr )
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

void ProtocolHelper::parseTagFetchResult( const QList<QByteArray> &lineTokens, Tag &tag )
{
  for (int i = 0; i < lineTokens.count() - 1; i += 2) {
    const QByteArray key = lineTokens.value(i);
    const QByteArray value = lineTokens.value(i + 1);

    if (key == "UID") {
      tag.setId(value.toLongLong());
    } else if (key == "GID") {
      tag.setGid(value);
    } else if (key == "REMOTEID") {
      tag.setRemoteId(value);
    } else if (key == "PARENT") {
      tag.setParent(Tag(value.toLongLong()));
    } else if ( key == "MIMETYPE" ) {
      tag.setType(value);
    } else {
      Attribute *attr = AttributeFactory::createAttribute(key);
      if (!attr) {
        kWarning() << "Unknown tag attribute" << key;
        continue;
      }
      attr->deserialize(value);
      tag.addAttribute(attr);
    }
  }
}

QString ProtocolHelper::akonadiStoragePath()
{
    QString fullRelPath = QLatin1String("akonadi");
    if (Akonadi::ServerManager::hasInstanceIdentifier()) {
        fullRelPath += QDir::separator() + QLatin1String("instance") + QDir::separator() + Akonadi::ServerManager::instanceIdentifier();
    }
    return XdgBaseDirs::saveDir("data", fullRelPath);
}

QString ProtocolHelper::absolutePayloadFilePath(const QString &fileName)
{
    QFileInfo fi(fileName);
    if (!fi.isAbsolute()) {
        return akonadiStoragePath() + QDir::separator() + QLatin1String("file_db_data") + QDir::separator() + fileName;
    }

    return fileName;
}

bool ProtocolHelper::streamPayloadToFile(const QByteArray &command, const QByteArray &data, QByteArray &error)
{
    const int fnStart = command.indexOf("[FILE ") + 6;
    if (fnStart == -1) {
        kDebug() << "Unexpected response";
        return false;
    }
    const int fnEnd = command.indexOf("]", fnStart);
    const QByteArray fn = command.mid(fnStart, fnEnd - fnStart);
    const QString fileName = ProtocolHelper::absolutePayloadFilePath(QString::fromLatin1(fn));
    if (!fileName.startsWith(akonadiStoragePath())) {
        kWarning() << "Invalid file path" << fileName;
        error = "Invalid file path";
        return false;
    }
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        kWarning() << "Failed to open destination payload file" << file.errorString();
        error = "Failed to store payload into file";
        return false;
    }
    if (file.write(data) != data.size()) {
        kWarning() << "Failed to write all payload data to file";
        error = "Failed to store payload into file";
        return false;
    }
    kDebug() << "Wrote" << data.size() << "bytes to " << file.fileName();

    // Make sure stuff is written to disk
    file.close();
    return true;
}


QByteArray ProtocolHelper::listPreference(Collection::ListPurpose purpose, Collection::ListPreference preference)
{
    QByteArray command;
    switch(purpose) {
    case Collection::ListDisplay:
        command += "DISPLAY ";
        break;
    case Collection::ListSync:
        command += "SYNC ";
        break;
    case Collection::ListIndex:
        command += "INDEX ";
        break;
    }
    switch(preference) {
    case Collection::ListEnabled:
        command += "TRUE";
        break;
    case Collection::ListDisabled:
        command += "FALSE";
        break;
    case Collection::ListDefault:
        command += "DEFAULT";
        break;
    }
    return command;
}

QByteArray ProtocolHelper::enabled(bool state)
{
    if (state) {
      return "ENABLED TRUE";
    }
    return "ENABLED FALSE";
}

QByteArray ProtocolHelper::referenced(bool state)
{
    if (state) {
      return "REFERENCED TRUE";
    }
    return "REFERENCED FALSE";
}
