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
#include <kstandarddirs.h>

#include <QtCore/QFile>
#include <QtCore/QDir>
#include <QtCore/QUuid>
#include <QtXml/QDomElement>

#ifdef HAVE_LIBXML2
#include <libxml/parser.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlschemas.h>
#endif

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

#ifdef HAVE_LIBXML2
  // schema validation
  XmlPtr<xmlDocPtr, xmlFreeDoc> sourceDoc( xmlParseMemory( data.constData(), data.length() ) );
  if ( !sourceDoc ) {
    emit status( Broken, i18n( "Unable to parse data file '%1'.", fileName ) );
    return;
  }

  const QString &schemaFileName = KGlobal::dirs()->findResource( "data", "akonadi_knut_resource/knut.xsd" );
  XmlPtr<xmlDocPtr, xmlFreeDoc> schemaDoc( xmlReadFile( schemaFileName.toLocal8Bit(), NULL, XML_PARSE_NONET ) );
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
#endif

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
  if ( Settings::self()->readOnly() )
    return;
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
    emit error( "No payload found for item '" + item.remoteId() + "'." );
    return false;
  }

  Item i( item );
  addItemPayload( i, l.at( 0 ).toElement() );
  itemRetrieved( i );
  return true;
}


void KnutResource::collectionAdded( const Akonadi::Collection &collection, const Akonadi::Collection &parent )
{
  QDomElement parentElem = findElementByRid( parent.remoteId() );
  if ( parentElem.isNull() ) {
    emit error( "Parent collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  Collection c( collection );
  const QDomElement newCol = serializeCollection( c );
  parentElem.insertBefore( newCol, QDomNode() );
  save();
  changeCommitted( c );
}

void KnutResource::collectionChanged( const Akonadi::Collection &collection )
{
  const QDomElement oldElem = findElementByRid( collection.remoteId() );
  if ( oldElem.isNull() ) {
    emit error( "Modified collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  Collection c( collection );
  const QDomElement newElem = serializeCollection( c );
  oldElem.parentNode().replaceChild( newElem, oldElem );
  save();
  changeCommitted( c );
}

void KnutResource::collectionRemoved( const Akonadi::Collection &collection )
{
  const QDomElement colElem = findElementByRid( collection.remoteId() );
  if ( colElem.isNull() ) {
    emit error( "Deleted collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  colElem.parentNode().removeChild( colElem );
  save();
  changeProcessed();
}


void KnutResource::itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection )
{
  QDomElement parentElem = findElementByRid( collection.remoteId() );
  if ( parentElem.isNull() ) {
    emit error( "Parent collection not found in DOM tree." );
    changeProcessed();
    return;
  }

  Item i( item );
  const QDomElement newItem = serializeItem( i );
  parentElem.appendChild( newItem );
  save();
  changeCommitted( i );
}

void KnutResource::itemChanged( const Akonadi::Item &item, const QSet<QByteArray>& )
{
  const QDomElement oldElem = findElementByRid( item.remoteId() );
  if ( oldElem.isNull() ) {
    emit error( "Modified item not found in DOM tree." );
    changeProcessed();
    return;
  }

  Item i( item );
  const QDomElement newElem = serializeItem( i );
  oldElem.parentNode().replaceChild( newElem, oldElem );
  save();
  changeCommitted( i );
}

void KnutResource::itemRemoved( const Akonadi::Item &item )
{
  const QDomElement itemElem = findElementByRid( item.remoteId() );
  if ( itemElem.isNull() ) {
    emit error( "Deleted item not found in DOM tree." );
    changeProcessed();
    return;
  }

  itemElem.parentNode().removeChild( itemElem );
  save();
  changeProcessed();
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


QDomElement KnutResource::serializeCollection( Akonadi::Collection &collection )
{
  if ( collection.remoteId().isEmpty() )
    collection.setRemoteId( QUuid::createUuid().toString() );
  QDomElement top = mDocument.createElement( "collection" );
  top.setAttribute( "rid", collection.remoteId() );
  top.setAttribute( "name", collection.name() );
  top.setAttribute( "content", collection.contentMimeTypes().join( "," ) );
  return top;
}

QDomElement KnutResource::serializeItem( Akonadi::Item &item )
{
  if ( item.remoteId().isEmpty() )
    item.setRemoteId( QUuid::createUuid().toString() );
  QDomElement top = mDocument.createElement( "item" );
  top.setAttribute( "rid", item.remoteId() );
  top.setAttribute( "mimetype", item.mimeType() );
  QDomElement payloadElem = mDocument.createElement( "payload" );
  QDomText payloadText = mDocument.createTextNode( QString::fromUtf8( item.payloadData() ) );
  payloadElem.appendChild( payloadText );
  top.appendChild( payloadElem );
  return top;
}


AKONADI_RESOURCE_MAIN( KnutResource )

#include "knutresource.moc"
