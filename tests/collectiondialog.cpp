/*
    Copyright (c) 2010 Tobias Koenig <tokoe@kde.org>

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

#include "collectiondialog.h"

#include <QtCore/QDebug>

#include <klocalizedstring.h>
#include <qapplication.h>
#include <QCommandLineParser>
#include <kaboutdata.h>



using namespace Akonadi;

int main( int argc, char **argv )
{
    QApplication app(argc, argv);
    KAboutData aboutData( QStringLiteral("test"),
                          i18n("Test Application"),
                          QLatin1String("1.0"));

    KAboutData::setApplicationData(aboutData);

    QCommandLineParser parser;
    parser.addVersionOption();
    parser.addHelpOption();
    aboutData.setupCommandLine(&parser);

    parser.process(app);
    aboutData.processCommandLine(&parser);

  CollectionDialog dlg;
  dlg.setMimeTypeFilter( QStringList() << QLatin1String( "text/directory" ) );
  dlg.setAccessRightsFilter( Collection::CanCreateItem );
  dlg.setDescription( i18n( "Select an address book for saving:" ) );
  dlg.setSelectionMode( QAbstractItemView::ExtendedSelection );
  dlg.changeCollectionDialogOptions( CollectionDialog::AllowToCreateNewChildCollection );
  dlg.exec();

  foreach ( const Collection &collection, dlg.selectedCollections() )
    qDebug() << "Selected collection:" << collection.name();

  return 0;
}
