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

#include <QDebug>
#include <QApplication>
#include <KAboutData>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <KLocalizedString>

using namespace Akonadi;

int main( int argc, char *argv[] )
{
  KAboutData aboutData( QStringLiteral("akonadi2xml"),
                        i18n( "Akonadi To XML converter" ),
                        QStringLiteral("1.0"),
                        i18n( "Converts an Akonadi collection subtree into a XML file." ),
                        KAboutLicense::GPL,
                        i18n( "(c) 2009 Volker Krause <vkrause@kde.org>" ) );

  QCommandLineParser parser;
  QApplication app(argc, argv);
  KAboutData::setApplicationData(aboutData);

  app.setApplicationName(aboutData.componentName());
  app.setApplicationDisplayName(aboutData.displayName());
  app.setOrganizationDomain(aboutData.organizationDomain());
  app.setApplicationVersion(aboutData.version());

  parser.addVersionOption();
  parser.addHelpOption();
  aboutData.setupCommandLine(&parser);
  parser.process(app);
  aboutData.processCommandLine(&parser);


  Collection root;
  if ( parser.isSet( QLatin1String("collection") ) ) {
    const QString path = parser.value( QLatin1String("collection") );
    CollectionPathResolver resolver( path );
    if ( !resolver.exec() ) {
      qCritical() << resolver.errorString();
      return -1;
    }
    root = Collection( resolver.collection() );
  } else
    return -1;

  XmlWriteJob writer( root, parser.value( QLatin1String("output") ) );
  if ( !writer.exec() ) {
    qCritical() << writer.exec();
    return -1;
  }
}

