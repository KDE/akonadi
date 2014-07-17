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

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocalizedstring.h>

using namespace Akonadi;

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, "test", 0, ki18n( "Test Application" ), "1.0", ki18n( "test app" ) );

  KApplication app;

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
