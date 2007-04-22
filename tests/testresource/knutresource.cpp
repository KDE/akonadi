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

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>

#include <libakonadi/collectionlistjob.h>
#include <libakonadi/collectionmodifyjob.h>
#include <libakonadi/itemappendjob.h>
#include <libakonadi/itemfetchjob.h>
#include <libakonadi/itemstorejob.h>
#include <libakonadi/session.h>

#include <kfiledialog.h>
#include <klocale.h>

#include "knutresource.h"

using namespace Akonadi;

KnutResource::KnutResource( const QString &id )
  : ResourceBase( id )
{
  mDataFile = settings()->value( "General/DataFile" ).toString();
}

KnutResource::~KnutResource()
{
}

void KnutResource::configure()
{
  QString oldFile = settings()->value( "General/DataFile" ).toString();
  KUrl url;
  if ( !oldFile.isEmpty() )
    url = KUrl::fromPath( oldFile );
  else
    url = KUrl::fromPath( QDir::homePath() );

  QString newFile = KFileDialog::getOpenFileName( url, "*.xml |Data File", 0, i18n( "Select Data File" ) );

  if ( newFile.isEmpty() )
    return;

  if ( oldFile == newFile )
    return;

  mDataFile = newFile;
  settings()->setValue( "General/DataFile", newFile );

  loadData();
  synchronize();
}

void KnutResource::aboutToQuit()
{
}

bool KnutResource::setConfiguration( const QString &config )
{
  mDataFile = config;
  settings()->setValue( "General/DataFile", mDataFile );

  loadData();
  synchronize();

  return true;
}

QString KnutResource::configuration() const
{
  return mDataFile;
}

bool KnutResource::requestItemDelivery( const DataReference &ref, int type, const QDBusMessage &msg )
{
  Q_UNUSED( type );

  const QString remoteId = ref.remoteId();

  QMutableMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();

    const CollectionEntry &entry = it.value();

    QByteArray data;

    if ( entry.addressees.contains( remoteId ) )
      data = mVCardConverter.createVCard( entry.addressees.value( remoteId ) );

    if ( entry.incidences.contains( remoteId ) )
      data = mICalConverter.toString( entry.incidences.value( remoteId ) ).toUtf8();

    if ( !data.isEmpty() ) {
      ItemStoreJob *job = new ItemStoreJob( ref, session() );
      job->setData( data );

      return deliverItem( job, msg );
    }
  }

  return false;
}

void KnutResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  if ( !mCollections.contains( collection.remoteId() ) )
    return;

  CollectionEntry &entry = mCollections[ collection.remoteId() ];

  if ( item.mimeType() == QLatin1String( "text/vcard" ) ) {
    const KABC::Addressee addressee = mVCardConverter.parseVCard( item.data() );
    if ( !addressee.isEmpty() ) {
      entry.addressees.insert( addressee.uid(), addressee );

      DataReference r( item.reference().id(), addressee.uid() );
      changesCommitted( r );
    }
  } else if ( item.mimeType() == QLatin1String( "text/calendar" ) ) {
    KCal::Incidence *incidence = mICalConverter.fromString( QString::fromUtf8( item.data() ) );
    if ( incidence ) {
      entry.incidences.insert( incidence->uid(), incidence );

      DataReference r( item.reference().id(), incidence->uid() );
      changesCommitted( r );
    }
  }
}

void KnutResource::itemChanged( const Akonadi::Item &item, const QStringList& )
{
  QMutableMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();

    CollectionEntry &entry = it.value();

    if ( entry.addressees.contains( item.reference().remoteId() ) ) {
      const KABC::Addressee addressee = mVCardConverter.parseVCard( item.data() );
      if ( !addressee.isEmpty() ) {
        entry.addressees.insert( addressee.uid(), addressee );

        DataReference r( item.reference().id(), addressee.uid() );
        changesCommitted( r );
      }

      return;
    } if ( entry.incidences.contains( item.reference().remoteId() ) ) {
      KCal::Incidence *incidence = mICalConverter.fromString( QString::fromUtf8( item.data() ) );
      if ( incidence ) {
        delete entry.incidences.take( item.reference().remoteId() );
        entry.incidences.insert( incidence->uid(), incidence );

        DataReference r( item.reference().id(), incidence->uid() );
        changesCommitted( r );
      }

      return;
    }
  }
}

void KnutResource::itemRemoved( const Akonadi::DataReference &reference )
{
  QMutableMapIterator<QString, CollectionEntry> it( mCollections );
  while ( it.hasNext() ) {
    it.next();

    CollectionEntry &entry = it.value();

    if ( entry.addressees.contains( reference.remoteId() ) ) {
      entry.addressees.remove( reference.remoteId() );
      return;
    } if ( entry.incidences.contains( reference.remoteId() ) ) {
      delete entry.incidences.take( reference.remoteId() );
      return;
    }
  }
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

void KnutResource::synchronizeCollection( const Akonadi::Collection &collection )
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
      if ( item.reference().remoteId() == uid ) {
        found = true;
        break;
      }
    }

    if ( found )
      continue;

    ItemAppendJob *append = new ItemAppendJob( collection, "text/vcard", session() );
    append->setRemoteId( uid );
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
      if ( item.reference().remoteId() == uid ) {
        found = true;
        break;
      }
    }

    if ( found )
      continue;

    ItemAppendJob *append = new ItemAppendJob( collection, "text/calendar", session() );
    append->setRemoteId( uid );
    if ( !append->exec() ) {
      changeProgress( 0 );
      changeStatus( Error, i18n( "Appending new calendar failed: %1", append->errorString() ) );
      return;
    }

    counter++;
    changeProgress( (counter * 100) / fullcount );
  }

  collectionSynchronized();
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
  collection.setRemoteId( mDataFile + "#" + element.attribute( "name" ) );
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
