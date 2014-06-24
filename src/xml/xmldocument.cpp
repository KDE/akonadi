/*
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

#include "xmldocument.h"
#include "format_p.h"
#include "xmlreader.h"

#include <KLocalizedString>

#include <qdom.h>
#include <qfile.h>

#ifdef HAVE_LIBXML2
#include <libxml/parser.h>
#include <libxml/xmlIO.h>
#include <libxml/xmlschemas.h>
#include <QStandardPaths>
#endif

//QT5 static const KCatalogLoader loader( QLatin1String( "libakonadi-xml") );

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
      return p != 0;
    }

  private:
    T p;
};

static QDomElement findElementByRidHelper( const QDomElement &elem, const QString &rid, const QString &elemName )
{
  if ( elem.isNull() )
    return QDomElement();
  if ( elem.tagName() == elemName && elem.attribute( Format::Attr::remoteId() ) == rid )
    return elem;
  const QDomNodeList children = elem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement child = children.at( i ).toElement();
    if ( child.isNull() )
      continue;
    const QDomElement rv = findElementByRidHelper( child, rid, elemName );
    if ( !rv.isNull() )
      return rv;
  }
  return QDomElement();
}

namespace Akonadi {

class XmlDocumentPrivate
{
  public:
    XmlDocumentPrivate() :
      valid( false )
    {
      lastError = i18n( "No data loaded." );
    }

    QDomElement findElementByRid( const QString &rid, const QString &elemName ) const
    {
      return findElementByRidHelper( document.documentElement(), rid, elemName );
    }

    QDomDocument document;
    QString lastError;
    bool valid;
};

}

XmlDocument::XmlDocument() :
  d( new XmlDocumentPrivate )
{
  const QDomElement rootElem = d->document.createElement( Format::Tag::root() );
  d->document.appendChild( rootElem );
}

XmlDocument::XmlDocument(const QString& fileName) :
  d( new XmlDocumentPrivate )
{
  loadFile( fileName );
}

XmlDocument::~XmlDocument()
{
  delete d;
}

bool Akonadi::XmlDocument::loadFile(const QString& fileName)
{
  d->valid = false;
  d->document = QDomDocument();

  if ( fileName.isEmpty() ) {
    d->lastError = i18n( "No filename specified" );
    return false;
  }

  QFile file( fileName );
  QByteArray data;
  if ( file.exists() ) {
    if ( !file.open( QIODevice::ReadOnly ) ) {
      d->lastError = i18n( "Unable to open data file '%1'.", fileName );
      return false;
    }
    data = file.readAll();
  } else {
    d->lastError = i18n( "File %1 does not exist.", fileName );
    return false;
  }

#ifdef HAVE_LIBXML2
  // schema validation
  XmlPtr<xmlDocPtr, xmlFreeDoc> sourceDoc( xmlParseMemory( data.constData(), data.length() ) );
  if ( !sourceDoc ) {
    d->lastError = i18n( "Unable to parse data file '%1'.", fileName );
    return false;
  }

  const QString &schemaFileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("akonadi/akonadi-xml.xsd") );
  XmlPtr<xmlDocPtr, xmlFreeDoc> schemaDoc( xmlReadFile( schemaFileName.toLocal8Bit().constData(), 0, XML_PARSE_NONET ) );
  if ( !schemaDoc ) {
    d->lastError = i18n( "Schema definition could not be loaded and parsed." );
    return false;
  }
  XmlPtr<xmlSchemaParserCtxtPtr, xmlSchemaFreeParserCtxt> parserContext( xmlSchemaNewDocParserCtxt( schemaDoc ) );
  if ( !parserContext ) {
    d->lastError = i18n( "Unable to create schema parser context." );
    return false;
  }
  XmlPtr<xmlSchemaPtr, xmlSchemaFree> schema( xmlSchemaParse( parserContext ) );
  if ( !schema ) {
    d->lastError = i18n( "Unable to create schema." );
    return false;
  }
  XmlPtr<xmlSchemaValidCtxtPtr, xmlSchemaFreeValidCtxt> validationContext( xmlSchemaNewValidCtxt( schema ) );
  if ( !validationContext ) {
    d->lastError = i18n( "Unable to create schema validation context." );
    return false;
  }

  if ( xmlSchemaValidateDoc( validationContext, sourceDoc ) != 0 ) {
    d->lastError = i18n( "Invalid file format." );
    return false;
  }
#endif

  // DOM loading
  QString errMsg;
  if ( !d->document.setContent( data, true, &errMsg ) ) {
    d->lastError = i18n( "Unable to parse data file: %1", errMsg );
    return false;
  }

  d->valid = true;
  d->lastError.clear();
  return true;
}

bool XmlDocument::writeToFile(const QString& fileName) const
{
  QFile f( fileName );
  if ( !f.open( QFile::WriteOnly ) ) {
    d->lastError = f.errorString();
    return false;
  }

  f.write( d->document.toByteArray( 2 ) );

  d->lastError.clear();
  return true;
}

bool XmlDocument::isValid() const
{
  return d->valid;
}

QString XmlDocument::lastError() const
{
  return d->lastError;
}

QDomDocument& XmlDocument::document() const
{
  return d->document;
}

QDomElement XmlDocument::collectionElement( const Collection &collection ) const
{
  if ( collection == Collection::root() )
    return d->document.documentElement();
  if ( collection.remoteId().isEmpty() )
    return QDomElement();
  if ( collection.parentCollection().remoteId().isEmpty() && collection.parentCollection() != Collection::root() )
    return d->findElementByRid( collection.remoteId(), Format::Tag::collection() );
  QDomElement parent = collectionElement( collection.parentCollection() );
  if ( parent.isNull() )
    return QDomElement();
  const QDomNodeList children = parent.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement child = children.at( i ).toElement();
    if ( child.isNull() )
      continue;
   if ( child.tagName() == Format::Tag::collection() && child.attribute( Format::Attr::remoteId() ) == collection.remoteId() )
    return child;
  }
  return QDomElement();
}

QDomElement XmlDocument::itemElementByRemoteId(const QString& rid) const
{
  return d->findElementByRid( rid, Format::Tag::item() );
}

Collection XmlDocument::collectionByRemoteId(const QString& rid) const
{
  const QDomElement elem = d->findElementByRid( rid, Format::Tag::collection() );
  return XmlReader::elementToCollection( elem );
}

Item XmlDocument::itemByRemoteId(const QString& rid, bool includePayload) const
{
  return XmlReader::elementToItem( itemElementByRemoteId( rid ), includePayload );
}

Collection::List XmlDocument::collections() const
{
  return XmlReader::readCollections( d->document.documentElement() );
}

Collection::List XmlDocument::childCollections( const Collection &parentCollection ) const
{
  QDomElement parentElem = collectionElement( parentCollection );

  if ( parentElem.isNull() ) {
    d->lastError = QLatin1String( "Parent node not found." );
    return Collection::List();
  }

  Collection::List rv;
  const QDomNodeList children = parentElem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement childElem = children.at( i ).toElement();
    if ( childElem.isNull() || childElem.tagName() != Format::Tag::collection() )
      continue;
    Collection c = XmlReader::elementToCollection( childElem );
    c.setParentCollection( parentCollection );
    rv.append( c );
  }

  return rv;
}


Item::List XmlDocument::items(const Akonadi::Collection& collection, bool includePayload) const
{
  const QDomElement colElem = collectionElement( collection );
  if ( colElem.isNull() ) {
    d->lastError = i18n( "Unable to find collection %1", collection.name() );
    return Item::List();
  } else {
    d->lastError.clear();
  }

  Item::List items;
  const QDomNodeList children = colElem.childNodes();
  for ( int i = 0; i < children.count(); ++i ) {
    const QDomElement itemElem = children.at( i ).toElement();
    if ( itemElem.isNull() || itemElem.tagName() != Format::Tag::item() )
      continue;
    items += XmlReader::elementToItem( itemElem, includePayload );
  }

  return items;
}
