/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>
    Copyright (c) 2009 Volker Krause <vkrause@kde.org>

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

#include <akonadi/changerecorder.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/itemfetchscope.h>

#include <kfiledialog.h>
#include <klocale.h>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtXml/QDomElement>

#include <libxml/parser.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlschemas.h>

using namespace Akonadi;

// helper class for dealing with libxml resource management
template <typename T, void FreeFunc(T)> class XmlPtr
{
  public:
    XmlPtr( const T &t ) : p( t ) {}
      
    ~XmlPtr()
    {
      FreeFunc( p );
    }

    operator T() const
    {
      return p;
    }

    operator bool() const
    {
      return p != NULL;
    }

  private:
    T p;
};


KnutResource::KnutResource( const QString &id )
  : ResourceBase( id )
{
  changeRecorder()->itemFetchScope().fetchFullPayload();
  changeRecorder()->fetchCollection( true );

  new SettingsAdaptor( Settings::self() );
  QDBusConnection::sessionBus().registerObject( QLatin1String( "/Settings" ),
                            Settings::self(), QDBusConnection::ExportAdaptors );
  connect( this, SIGNAL(reloadConfiguration()), SLOT(load()) );
  load();
}

KnutResource::~KnutResource()
{
}

void KnutResource::load()
{
  // file loading
  const QString fileName = Settings::self()->dataFile();
  if ( fileName.isEmpty() ) {
    emit status( Broken, i18n( "No data file selected." ) );
    return;
  }
  QFile file( fileName );
  if ( !file.open( QIODevice::ReadOnly ) ) {
    emit status( Broken, i18n( "Unable to open data file '%1'.", fileName ) );
    return;
  }
  const QByteArray data = file.readAll();

  // schema validation
  XmlPtr<xmlDocPtr, xmlFreeDoc> sourceDoc( xmlParseMemory( data.constData(), data.length() ) );
  if ( !sourceDoc ) {
    emit status( Broken, i18n( "Unable to parse data file '%1'.", fileName ) );
    return;
  }

  XmlPtr<xmlDocPtr, xmlFreeDoc> schemaDoc( xmlReadFile( "/k/kde4/src/playground/pim/akonadi/knut/knut.xsd", NULL, XML_PARSE_NONET ) );
  if ( !schemaDoc ) {
    emit status( Broken, "Schema definition could not be loaded and parsed." );
    return;
  }
  XmlPtr<xmlSchemaParserCtxtPtr, xmlSchemaFreeParserCtxt> parserContext( xmlSchemaNewDocParserCtxt( schemaDoc ) );
  if ( !parserContext ) {
    emit status( Broken, "Unable to create schema parser context." );
    return;
  }
  XmlPtr<xmlSchemaPtr, xmlSchemaFree> schema( xmlSchemaParse( parserContext ) );
  if ( !schema ) {
    emit status( Broken, "Unable to create schema." );
    return;
  }
  XmlPtr<xmlSchemaValidCtxtPtr, xmlSchemaFreeValidCtxt> validationContext( xmlSchemaNewValidCtxt( schema ) );
  if ( !validationContext ) {
    emit status( Broken, "Unable to create schema validation context." );
    return;
  }

  if ( xmlSchemaValidateDoc( validationContext, sourceDoc ) != 0 ) {
    emit status( Broken, i18n( "Invalid file format." ) );
    return;
  }

  // DOM loading
  QString errMsg;
  if ( !mDocument.setContent( data, true, &errMsg ) ) {
    emit status( Broken, i18n( "Unable to parse data file: %1", errMsg ) );
    return;
  }

  emit status( Idle, i18n( "File '%1' loaded successfully.", fileName ) );
}

void KnutResource::save()
{
  const QString fileName = Settings::self()->dataFile();
  QFile file( fileName );
  if  ( !file.open( QIODevice::WriteOnly ) ) {
    emit error( i18n( "Unable to write to file '%1'.", fileName ) );
    return;
  }
  file.write( mDocument.toByteArray( 2 ) );
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

  Settings::self()->setDataFile( newFile );
  Settings::self()->writeConfig();
  load();
  if ( status() != Broken )
    synchronize();
}


#if 0
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
#endif


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
