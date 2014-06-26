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

#include "xmlwritejob.h"

#include "collection.h"
#include "collectionpathresolver.h"

#include <k4aboutdata.h>
#include <KApplication>
#include <KCmdLineArgs>
#include <QDebug>

using namespace Akonadi;

int main( int argc, char *argv[] )
{
  K4AboutData aboutdata( "akonadi2xml", 0,
                        ki18n( "Akonadi To XML converter" ),
                        "1.0",
                        ki18n( "Converts an Akonadi collection subtree into a XML file." ),
                        K4AboutData::License_GPL,
                        ki18n( "(c) 2009 Volker Krause <vkrause@kde.org>" ) );

  KCmdLineArgs::init( argc, argv, &aboutdata );
  KCmdLineOptions options;
  options.add( "c" ).add( "collection <root>", ki18n( "Root collection id or path" ) );
  options.add( "o" ).add( "output <file>", ki18n( "Output file" ) );
  KCmdLineArgs::addCmdLineOptions( options );

  KApplication app;
  const KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  Collection root;
  if ( args->isSet( "collection" ) ) {
    const QString path = args->getOption( "collection" );
    CollectionPathResolver resolver( path );
    if ( !resolver.exec() ) {
      qCritical() << resolver.errorString();
      return -1;
    }
    root = Collection( resolver.collection() );
  } else
    return -1;

  XmlWriteJob writer( root, args->getOption( "output" ) );
  if ( !writer.exec() ) {
    qCritical() << writer.exec();
    return -1;
  }
}
