/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "configreader.h"
#include "config.h"
#include "symbols.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>

ConfigReader::ConfigReader( const QString &configfile )
{
  QDomDocument doc;
  QFile file( configfile );

  if ( !file.open( QIODevice::ReadOnly ) )
    qFatal( "error reading file: %s", qPrintable( configfile ) );

  QString errorMsg;
  if ( !doc.setContent( &file, &errorMsg ) )
    qFatal( "unable to parse config file: %s", qPrintable( errorMsg ) );

  const QDomElement root = doc.documentElement();
  if ( root.tagName() != "config" )
    qFatal( "could not file root tag" );

  const QString basePath = QFileInfo( configfile ).absolutePath() + "/";

  QDomNode node = root.firstChild();
  while ( !node.isNull() ) {
    const QDomElement element = node.toElement();
    if ( !element.isNull() ) {
      if ( element.tagName() == "kdehome" ) {
        setKdeHome( basePath + element.text() );
      } else if ( element.tagName() == "confighome" ) {
        setXdgConfigHome( basePath + element.text() );
      } else if ( element.tagName() == "datahome" ) {
        setXdgDataHome( basePath + element.text() );
      } else if ( element.tagName() == "item" ) {
        insertItemConfig( element.attribute( "location" ), element.attribute( "collection" ) );
      } else if ( element.tagName() == "agent" ) {
        insertAgent( element.text(), element.attribute( "synchronize", "false" ) == QLatin1String("true") );
      }
    }

    node = node.nextSibling();
  }
}
