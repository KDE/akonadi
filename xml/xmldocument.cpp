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

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KStandardDirs>

#include <qdom.h>
#include <qfile.h>

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
    {}

    QDomElement findElementByRid( const QString &rid, const QString &elemName ) const
    {
      return findElementByRidHelper( document.documentElement(), rid, elemName );
    }

    QDomDocument document;
    QString lastError;
    bool valid;
};

}

XmlDocument::XmlDocument(const QString& fileName) :
  d( new XmlDocumentPrivate )
{
  if ( fileName.isEmpty() )
    return;

  QFile file( fileName );
  QByteArray data;
  if ( file.exists() ) {
    if ( !file.open( QIODevice::ReadOnly ) ) {
      d->lastError = i18n( "Unable to open data file '%1'.", fileName );
      return;
    }
    data = file.readAll();
  } else {
    const QString tmplFilename = KGlobal::dirs()->findResource( "data", "akonadi_knut_resource/knut-template.xml" );
    QFile tmpl( tmplFilename );
    if ( !tmpl.open( QFile::ReadOnly ) ) {
      d->lastError = i18n( "Unable to open template file '%1'.", tmplFilename );
      return;
    }
    data = tmpl.readAll();
  }

#ifdef HAVE_LIBXML2
  // schema validation
  XmlPtr<xmlDocPtr, xmlFreeDoc> sourceDoc( xmlParseMemory( data.constData(), data.length() ) );
  if ( !sourceDoc ) {
    d->lastError = i18n( "Unable to parse data file '%1'.", fileName );
    return;
  }

  const QString &schemaFileName = KGlobal::dirs()->findResource( "data", "akonadi_knut_resource/knut.xsd" );
  XmlPtr<xmlDocPtr, xmlFreeDoc> schemaDoc( xmlReadFile( schemaFileName.toLocal8Bit(), NULL, XML_PARSE_NONET ) );
  if ( !schemaDoc ) {
    d->lastError = i18n( "Schema definition could not be loaded and parsed." );
    return;
  }
  XmlPtr<xmlSchemaParserCtxtPtr, xmlSchemaFreeParserCtxt> parserContext( xmlSchemaNewDocParserCtxt( schemaDoc ) );
  if ( !parserContext ) {
    d->lastError = i18n( "Unable to create schema parser context." );
    return;
  }
  XmlPtr<xmlSchemaPtr, xmlSchemaFree> schema( xmlSchemaParse( parserContext ) );
  if ( !schema ) {
    d->lastError = i18n( "Unable to create schema." );
    return;
  }
  XmlPtr<xmlSchemaValidCtxtPtr, xmlSchemaFreeValidCtxt> validationContext( xmlSchemaNewValidCtxt( schema ) );
  if ( !validationContext ) {
    d->lastError = i18n( "Unable to create schema validation context." );
    return;
  }

  if ( xmlSchemaValidateDoc( validationContext, sourceDoc ) != 0 ) {
    d->lastError = i18n( "Invalid file format." );
    return;
  }
#endif

  // DOM loading
  QString errMsg;
  if ( !d->document.setContent( data, true, &errMsg ) ) {
    d->lastError = i18n( "Unable to parse data file: %1", errMsg );
    return;
  }

  d->valid = true;
}

XmlDocument::~XmlDocument()
{
  delete d;
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

QDomElement XmlDocument::collectionElementByRemoteId(const QString& rid) const
{
  return d->findElementByRid( rid, Format::Tag::collection() );
}

QDomElement XmlDocument::itemElementByRemoteId(const QString& rid) const
{
  return d->findElementByRid( rid, Format::Tag::item() );
}

Collection XmlDocument::collectionByRemoteId(const QString& rid) const
{
  const QDomElement elem = collectionElementByRemoteId( rid );
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

Item::List XmlDocument::items(const Akonadi::Collection& collection, bool includePayload) const
{
  const QDomElement colElem = collectionElementByRemoteId( collection.remoteId() );
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
