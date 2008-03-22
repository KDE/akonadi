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
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <akonadi/collectionlistjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/itemappendjob.h>
#include <akonadi/itemfetchjob.h>
#include <akonadi/itemstorejob.h>
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

bool KnutResource::retrieveItem( const Item &item, const QStringList &parts )
{
  Q_UNUSED( parts );

  const QString remoteId = item.remoteId();

  QMutableMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();

    const CollectionEntry &entry = it.value();
    Item i( item );

    if ( entry.addressees.contains( remoteId ) ) {
      i.setMimeType( "text/vcard" );
      i.setPayload<KABC::Addressee>( entry.addressees.value( remoteId ) );
    }

    if ( entry.incidences.contains( remoteId ) ) {
      i.setMimeType( "text/calendar" );
      i.setPayload<IncidencePtr>( IncidencePtr( entry.incidences.value( remoteId ) ) );
    }

    if ( i.hasPayload() ) {
      itemRetrieved( i );
      return true;
    }
  }

  return false;
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
      changesCommitted( i );
      return;
    }
  } else if ( item.mimeType() == QLatin1String( "text/calendar" ) ) {
    KCal::Incidence *incidence = mICalConverter.fromString( QString::fromUtf8( item.payload<QByteArray>() ) );
    if ( incidence ) {
      entry.incidences.insert( incidence->uid(), incidence );

      Item i( item );
      i.setRemoteId( incidence->uid() );
      changesCommitted( i );
      return;
    }
  }
  changeProcessed();
}

void KnutResource::itemChanged( const Akonadi::Item &item, const QStringList& )
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
        changesCommitted( i );
        return;
      }
    } else if ( entry.incidences.contains( item.remoteId() ) ) {
      KCal::Incidence *incidence = mICalConverter.fromString( item.payload<QByteArray>() );
      if ( incidence ) {
        delete entry.incidences.take( item.remoteId() );
        entry.incidences.insert( incidence->uid(), incidence );

        Item i( item );
        i.setRemoteId( incidence->uid() );
        changesCommitted( i );
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

void KnutResource::retrieveCollections()
{
  Akonadi::Collection::List collections;
  QMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();
    collections.append( it.value().collection );
  }

  collectionsRetrieved( collections );
}

void KnutResource::retrieveItems( const Akonadi::Collection &collection, const QStringList &parts )
{
  ItemFetchJob *fetch = new ItemFetchJob( collection, session() );
  if ( !fetch->exec() ) {
    changeStatus( Error, i18n( "Unable to fetch listing of collection '%1': %2", collection.name(), fetch->errorString() ) );
    return;
  }

  changeProgress( 0 );

  Item::List items = fetch->items();

  KABC::Addressee::List addressees;
  KCal::Incidence::List incidences;
  QMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();
    if ( it.value().collection.id() == collection.id() ) {
      addressees = it.value().addressees.values();

      const QList<KCal::Incidence*> list = it.value().incidences.values();
      for ( int i = 0; i < list.count(); ++i )
        incidences.append( list[ i ] );
      break;
    }
  }

  int counter = 0;
  int fullcount = addressees.count() + incidences.count();

  // synchronize contacts
  foreach ( KABC::Addressee addressee, addressees ) {
    const QString uid = addressee.uid();

    bool found = false;
    foreach ( Item item, items ) {
      if ( item.remoteId() == uid ) {
        found = true;
        break;
      }
    }

    if ( found )
      continue;

    Item item;
    item.setRemoteId( uid );
    item.setMimeType( "text/vcard" );
    ItemAppendJob *append = new ItemAppendJob( item, collection, session() );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n( "Appending new contact failed: %1", append->errorString() ) );
      return;
    }

    counter++;
    changeProgress( (counter * 100) / fullcount );
  }

  // synchronize events
  foreach ( KCal::Incidence *incidence, incidences ) {
    const QString uid = incidence->uid();

    bool found = false;
    foreach ( Item item, items ) {
      if ( item.remoteId() == uid ) {
        found = true;
        break;
      }
    }

    if ( found )
      continue;

    Item item;
    item.setRemoteId( uid );
    item.setMimeType( "text/calendar" );
    ItemAppendJob *append = new ItemAppendJob( item, collection, session() );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n( "Appending new calendar failed: %1", append->errorString() ) );
      return;
    }

    counter++;
    changeProgress( (counter * 100) / fullcount );
  }

  itemsRetrieved();
}

bool KnutResource::loadData()
{
  mCollections.clear();

  QFile file( mDataFile );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    changeStatus( Error, "Unable to open data file" );
    return false;
  }

  QDomDocument document;
  if ( !document.setContent( &file, true ) ) {
    changeStatus( Error, "Unable to parse data file" );
    return false;
  }

  QDomElement element = document.documentElement();
  if ( element.tagName() != QLatin1String( "knut" ) ) {
    changeStatus( Error, "Data file has invalid format" );
    return false;
  }

  Collection parentCollection;
  parentCollection.setParent( Collection::root() );
  parentCollection.setRemoteId( mDataFile );
  parentCollection.setName( name() );
  QStringList contentTypes;
  contentTypes << Collection::collectionMimeType();
  parentCollection.setContentTypes( contentTypes );

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
  collection.setContentTypes( element.attribute( "mimetypes" ).split( ";", QString::SkipEmptyParts ) );

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

#include "knutresource.moc"
