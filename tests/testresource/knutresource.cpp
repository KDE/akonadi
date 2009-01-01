/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include "knutresource.h"
#include "settings.h"
#include "settingsadaptor.h"

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtXml/QDomElement>

#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/session.h>

#include <kfiledialog.h>
#include <klocale.h>

#include <boost/shared_ptr.hpp>

using namespace Akonadi;

typedef boost::shared_ptr<KCal::Incidence> IncidencePtr;

KnutResource::KnutResource( const QString &id )
  : ResourceBase( id )
{
  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
  mDataFile = Settings::self()->dataFile();
}

KnutResource::~KnutResource()
{
}

void KnutResource::configure( WId windowId )
{
  QString newFile;

  QString oldFile = Settings::self()->dataFile();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );

  newFile = KFileDialog::getOpenFileNameWId( url, "*.xml |" + i18nc( "Filedialog filter for Akonadi data file", "Akonadi Knut Data File" ), windowId, i18n( "Select Data File" ) );

  if ( newFile.isEmpty() || oldFile == newFile )
    return;

  setConfiguration( newFile );
}

void KnutResource::aboutToQuit()
{
}

bool KnutResource::setConfiguration( const QString &config )
{
  mDataFile = config;
  Settings::self()->setDataFile( mDataFile );

  loadData();
  synchronize();

  return true;
}

QString KnutResource::configuration() const
{
  return mDataFile;
}

void KnutResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( !mCollections.contains( collection.remoteId() ) ) {
    changeProcessed();
    return;
  }

  CollectionEntry &entry = mCollections[ collection.remoteId() ];

  if ( item.mimeType() == QLatin1String( "text/vcard" ) ) {
    const KABC::Addressee addressee = item.payload<KABC::Addressee>();
    if ( !addressee.isEmpty() ) {
      entry.addressees.insert( addressee.uid(), addressee );

      Item i( item );
      i.setRemoteId( addressee.uid() );
      changeCommitted( i );
      return;
    }
  } else if ( item.mimeType() == QLatin1String( "text/calendar" ) ) {
    KCal::Incidence *incidence = mICalConverter.fromString( QString::fromUtf8( item.payload<QByteArray>() ) );
    if ( incidence ) {
      entry.incidences.insert( incidence->uid(), incidence );

      Item i( item );
      i.setRemoteId( incidence->uid() );
      changeCommitted( i );
      return;
    }
  }
  changeProcessed();
}

void KnutResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  QMutableMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();

    CollectionEntry &entry = it.value();

    if ( entry.addressees.contains( item.remoteId() ) ) {
      const KABC::Addressee addressee = item.payload<KABC::Addressee>();
      if ( !addressee.isEmpty() ) {
        entry.addressees.insert( addressee.uid(), addressee );

        Item i( item );
        i.setRemoteId( addressee.uid() );
        changeCommitted( i );
        return;
      }
    } else if ( entry.incidences.contains( item.remoteId() ) ) {
      KCal::Incidence *incidence = mICalConverter.fromString( item.payload<QByteArray>() );
      if ( incidence ) {
        delete entry.incidences.take( item.remoteId() );
        entry.incidences.insert( incidence->uid(), incidence );

        Item i( item );
        i.setRemoteId( incidence->uid() );
        changeCommitted( i );
        return;
      }
    }
  }
  changeProcessed();
}

void KnutResource::itemRemoved( const Akonadi::Item &item )
{
  QMutableMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();

    CollectionEntry &entry = it.value();

    if ( entry.addressees.contains( item.remoteId() ) ) {
      entry.addressees.remove( item.remoteId() );
    } else if ( entry.incidences.contains( item.remoteId() ) ) {
      delete entry.incidences.take( item.remoteId() );
    }
  }
  changeProcessed();
}


static Collection buildCollection( const QDomElement &node, const QString &parentRid )
{
  Collection c;
  c.setRemoteId( node.attribute( "rid" ) );
  c.setParentRemoteId( parentRid );
  c.setName( node.attribute( "name" ) );
  c.setContentMimeTypes( node.attribute( "content" ).split( ',' ) );
  return c;
}

static Collection::List buildCollectionTree( const QDomElement &parent )
{
  Collection::List rv;
  if ( parent.isNull() )
    return rv;
  const QDomNodeList children = parent.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement child = children.at( i ).toElement();
    if ( child.isNull() || child.tagName() != "collection" )
      continue;
    rv += buildCollection( child, parent.attribute( "rid" ) );
    rv += buildCollectionTree( child );
  }
  return rv;
}

void KnutResource::retrieveCollections()
{
  const Collection::List collections = buildCollectionTree( mDocument.documentElement() );
  collectionsRetrieved( collections );
}


static Item buildItem( const QDomElement &elem )
{
  Item i( elem.attribute( "mimetype", "application/octet-stream" ) );
  i.setRemoteId( elem.attribute( "rid" ) );
  return i;
}

void KnutResource::retrieveItems( const Akonadi::Collection &collection )
{
  const QDomElement colElem = findElementByRid( collection.remoteId() );
  if ( colElem.isNull() ) {
    emit error( "Unable to find collection " + collection.name() );
    itemsRetrievalDone();
    return;
  }

  Item::List items;
  const QDomNodeList children = colElem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement itemElem = children.at( i ).toElement();
    if ( itemElem.isNull() || itemElem.tagName() != "item" )
      continue;
    items += buildItem( itemElem );
  }

  itemsRetrieved( items );
}


static void addItemPayload( Item &item, const QDomElement &elem )
{
  kDebug() << elem.tagName() << elem.text();
  if ( elem.isNull() )
    return;
  const QByteArray payloadData = elem.text().toUtf8();
  item.setPayloadFromData( payloadData );
}

bool KnutResource::retrieveItem( const Item &item, const QSet<QByteArray> &parts )
{
  Q_UNUSED( parts );

  const QDomElement itemElem = findElementByRid( item.remoteId() );
  if ( itemElem.isNull() ) {
    emit error( "No item found for remoteid " + item.remoteId() );
    return false;
  }

  const QDomNodeList l = itemElem.elementsByTagName( "payload" );
  if ( l.count() != 1 ) {
    emit error( "No payload found for item " + item.remoteId() );
    return false;
  }

  Item i( item );
  addItemPayload( i, l.at( 0 ).toElement() );
  itemRetrieved( i );
  return true;
}


bool KnutResource::loadData()
{
  mCollections.clear();

  QFile file( mDataFile );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    emit status( Broken, "Unable to open data file" );
    return false;
  }

  if ( !mDocument.setContent( &file, true ) ) {
    emit status( Broken, "Unable to parse data file" );
    return false;
  }

  QDomElement element = mDocument.documentElement();
  if ( element.tagName() != QLatin1String( "knut" ) ) {
    emit status( Broken, "Data file has invalid format" );
    return false;
  }

  Collection parentCollection;
  parentCollection.setParent( Collection::root() );
  parentCollection.setRemoteId( mDataFile );
  parentCollection.setName( name() );
  QStringList contentTypes;
  contentTypes << Collection::mimeType();
  parentCollection.setContentMimeTypes( contentTypes );

  CollectionEntry entry;
  entry.collection = parentCollection;

  element = element.firstChildElement();
  while ( !element.isNull() ) {
    if ( element.tagName() == QLatin1String( "collection" ) ) {
      addCollection( element, parentCollection );
    } else if ( element.tagName() == QLatin1String( "item" ) ) {
      if ( element.attribute( "mimetype" ) == QLatin1String( "text/vcard" ) )
        addAddressee( element, entry );
      else if ( element.attribute( "mimetype" ) == QLatin1String( "text/calendar" ) )
        addIncidence( element, entry );
    }

    element = element.nextSiblingElement();
  }

  mCollections.insert( entry.collection.remoteId(), entry );

  return true;
}

void KnutResource::addCollection( const QDomElement &element, const Akonadi::Collection &parentCollection )
{
  Collection collection;
  collection.setParent( parentCollection );
  collection.setName( element.attribute( "name" ) );
  collection.setRemoteId( mDataFile + QLatin1Char( '#' ) + element.attribute( "name" ) );
  collection.setContentMimeTypes( element.attribute( "mimetypes" ).split( ";", QString::SkipEmptyParts ) );

  CollectionEntry entry;
  entry.collection = collection;

  QDomElement childElement = element.firstChildElement();
  while ( !childElement.isNull() ) {
    if ( childElement.tagName() == QLatin1String( "collection" ) ) {
      addCollection( childElement, collection );
    } else if ( childElement.tagName() == QLatin1String( "item" ) ) {
      if ( childElement.attribute( "mimetype" ) == QLatin1String( "text/x-vcard" ) )
        addAddressee( childElement, entry );
      else if ( childElement.attribute( "mimetype" ) == QLatin1String( "text/x-calendar" ) )
        addIncidence( childElement, entry );
    }

    childElement = childElement.nextSiblingElement();
  }

  mCollections.insert( entry.collection.remoteId(), entry );
}

void KnutResource::addAddressee( const QDomElement &element, CollectionEntry &entry )
{
  const QString data = element.text();

  KABC::Addressee addressee = mVCardConverter.parseVCard( data.toUtf8() );
  if ( !addressee.isEmpty() )
    entry.addressees.insert( addressee.uid(), addressee );
}

void KnutResource::addIncidence( const QDomElement &element, CollectionEntry &entry )
{
  const QString data = element.text();

  KCal::Incidence *incidence = mICalConverter.fromString( data );
  if ( incidence )
    entry.incidences.insert( incidence->uid(), incidence );
}


static QDomElement findElementByRidHelper( const QDomElement &elem, const QString &rid )
{
  if ( elem.isNull() )
    return QDomElement();
  if ( elem.attribute( "rid" ) == rid )
    return elem;
  const QDomNodeList children = elem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement child = children.at( i ).toElement();
    if ( child.isNull() )
      continue;
    const QDomElement rv = findElementByRidHelper( child, rid );
    if ( !rv.isNull() )
      return rv;
  }
  return QDomElement();
}

QDomElement KnutResource::findElementByRid( const QString &rid ) const
{
  return findElementByRidHelper( mDocument.documentElement(), rid );
}


AKONADI_RESOURCE_MAIN( KnutResource )

#include "knutresource.moc"
