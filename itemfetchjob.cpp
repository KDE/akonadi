/*
    Copyright (c) 2006 - 2007 Volker Krause <vkrause@kde.org>

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

#include "itemfetchjob.h"

#include "attributefactory.h"
#include "collection.h"
#include "collectionselectjob_p.h"
#include "imapparser_p.h"
#include "itemfetchscope.h"
#include "itemserializer_p.h"
#include "itemserializerplugin.h"
#include "job_p.h"
#include "entity_p.h"
#include "protocol_p.h"
#include "protocolhelper_p.h"

#include <kdebug.h>

#include <QtCore/QDateTime>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QFile>

using namespace Akonadi;

class Akonadi::ItemFetchJobPrivate : public JobPrivate
{
  public:
    ItemFetchJobPrivate( ItemFetchJob *parent )
      : JobPrivate( parent )
    {
    }

    void timeout()
    {
      Q_Q( ItemFetchJob );

      mEmitTimer->stop(); // in case we are called by result()
      if ( !mPendingItems.isEmpty() ) {
        emit q->itemsReceived( mPendingItems );
        mPendingItems.clear();
      }
    }

    void startFetchJob();
    void selectDone( KJob * job );

    Q_DECLARE_PUBLIC( ItemFetchJob )

    Collection mCollection;
    Item mItem;
    Item::List mItems;
    ItemFetchScope mFetchScope;
    Item::List mPendingItems; // items pending for emitting itemsReceived()
    QTimer* mEmitTimer;
};

void ItemFetchJobPrivate::startFetchJob()
{
  QByteArray command = newTag();
  if ( mItem.isValid() )
    command += " " AKONADI_CMD_UID " " AKONADI_CMD_ITEMFETCH " " + QByteArray::number( mItem.id() );
  else if ( !mItem.remoteId().isEmpty() )
    command += " " AKONADI_CMD_RID " " AKONADI_CMD_ITEMFETCH " " + mItem.remoteId().toUtf8();
  else
    command += " " AKONADI_CMD_ITEMFETCH " 1:*";

  if ( mFetchScope.fullPayload() )
    command += " " AKONADI_PARAM_FULLPAYLOAD;
  if ( mFetchScope.allAttributes() )
    command += " " AKONADI_PARAM_ALLATTRIBUTES;
  if ( mFetchScope.cacheOnly() )
    command += " " AKONADI_PARAM_CACHEONLY;

  //TODO: detect somehow if server supports external payload attribute
  command += " " AKONADI_PARAM_EXTERNALPAYLOAD;

  command += " (UID REMOTEID COLLECTIONID FLAGS SIZE DATETIME";
  foreach ( const QByteArray &part, mFetchScope.payloadParts() )
    command += ' ' + ProtocolHelper::encodePartIdentifier( ProtocolHelper::PartPayload, part );
  foreach ( const QByteArray &part, mFetchScope.attributes() )
    command += ' ' + ProtocolHelper::encodePartIdentifier( ProtocolHelper::PartAttribute, part );
  command += ")\n";

  writeData( command );
}

void ItemFetchJobPrivate::selectDone( KJob * job )
{
  if ( !job->error() )
    // the collection is now selected, fetch the message(s)
    startFetchJob();
}

ItemFetchJob::ItemFetchJob( const Collection &collection, QObject * parent )
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );

  d->mCollection = collection;
}

ItemFetchJob::ItemFetchJob( const Item & item, QObject * parent)
  : Job( new ItemFetchJobPrivate( this ), parent )
{
  Q_D( ItemFetchJob );

  d->mEmitTimer = new QTimer( this );
  d->mEmitTimer->setSingleShot( true );
  d->mEmitTimer->setInterval( 100 );
  connect( d->mEmitTimer, SIGNAL(timeout()), this, SLOT(timeout()) );
  connect( this, SIGNAL(result(KJob*)), this, SLOT(timeout()) );

  d->mCollection = Collection::root();
  d->mItem = item;
}

ItemFetchJob::~ItemFetchJob()
{
}

void ItemFetchJob::doStart()
{
  Q_D( ItemFetchJob );

  if ( !d->mItem.isValid() ) { // collection content listing
    if ( d->mCollection == Collection::root() ) {
      setErrorText( QLatin1String("Cannot list root collection.") );
      setError( Unknown );
      emitResult();
    }
    CollectionSelectJob *job = new CollectionSelectJob( d->mCollection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(selectDone(KJob*)) );
    addSubjob( job );
  } else
    d->startFetchJob();
}

void ItemFetchJob::doHandleResponse( const QByteArray & tag, const QByteArray & data )
{
  Q_D( ItemFetchJob );

  if ( tag == "*" ) {
    int begin = data.indexOf( "FETCH" );
    if ( begin >= 0 ) {

      // split fetch response into key/value pairs
      QList<QByteArray> fetchResponse;
      ImapParser::parseParenthesizedList( data, fetchResponse, begin + 6 );

      // create a new item object
      Item::Id uid = -1;
      int rev = -1;
      QString rid;
      QString mimeType;
      Entity::Id cid = -1;

      for ( int i = 0; i < fetchResponse.count() - 1; i += 2 ) {
        const QByteArray key = fetchResponse.value( i );
        const QByteArray value = fetchResponse.value( i + 1 );

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
        kWarning( 5250 ) << "Broken fetch response: UID, RID, REV or MIMETYPE missing!";
        return;
      }

      Item item( uid );
      item.setRemoteId( rid );
      item.setRevision( rev );
      item.setMimeType( mimeType );
      item.setStorageCollectionId( cid );
      if ( !item.isValid() )
        return;

      // parse fetch response fields
      for ( int i = 0; i < fetchResponse.count() - 1; i += 2 ) {
        const QByteArray key = fetchResponse.value( i );
        // skip stuff we dealt with already
        if ( key == "UID" || key == "REV" || key == "REMOTEID" || key == "MIMETYPE"  || key == "COLLECTIONID")
          continue;
        // flags
        if ( key == "FLAGS" ) {
          QList<QByteArray> flags;
          ImapParser::parseParenthesizedList( fetchResponse[i + 1], flags );
          foreach ( const QByteArray &flag, flags ) {
            item.setFlag( flag );
          }
        } else if ( key == "SIZE" ) {
          const quint64 size = fetchResponse[i + 1].toLongLong();
          item.setSize( size );
        } else if ( key == "DATETIME" ) {
          QDateTime datetime;
          ImapParser::parseDateTime( fetchResponse[i + 1], datetime );
          item.setModificationTime( datetime );
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
              QByteArray fileKey = fetchResponse.value( i + 1 );
              if (fileKey == "[FILE]") {
                isExternal = true;
                i++;
                kDebug( 5250 ) << "Payload is external: " << isExternal << " filename: " << fetchResponse.value( i + 1 );
              }
              ItemSerializer::deserialize( item, plainKey, fetchResponse.value( i + 1 ), version, isExternal );
              break;
            }
            case ProtocolHelper::PartAttribute:
            {
              Attribute* attr = AttributeFactory::createAttribute( plainKey );
              Q_ASSERT( attr );
              if ( fetchResponse.value( i + 1 ) == "[FILE]" ) {
                ++i;
                QFile f( QString::fromUtf8( fetchResponse.value( i + 1 ) ) );
                if ( f.open( QFile::ReadOnly ) )
                  attr->deserialize( f.readAll() );
                else {
                  kWarning() << "Failed to open attribute file: " << fetchResponse.value( i + 1 );
                  delete attr;
                }
              } else {
                attr->deserialize( fetchResponse.value( i + 1 ) );
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
      d->mItems.append( item );
      d->mPendingItems.append( item );
      if ( !d->mEmitTimer->isActive() )
        d->mEmitTimer->start();
      return;
    }
  }
  kDebug( 5250 ) << "Unhandled response: " << tag << data;
}

Item::List ItemFetchJob::items() const
{
  Q_D( const ItemFetchJob );

  return d->mItems;
}

void ItemFetchJob::setFetchScope( ItemFetchScope &fetchScope )
{
  Q_D( ItemFetchJob );

  d->mFetchScope = fetchScope;
}

ItemFetchScope &ItemFetchJob::fetchScope()
{
  Q_D( ItemFetchJob );

  return d->mFetchScope;
}

void ItemFetchJob::setCollection(const Akonadi::Collection& collection)
{
  Q_D( ItemFetchJob );
  d->mCollection = collection;
}


#include "itemfetchjob.moc"
