/*
    Copyright (c) 2010 KDAB
    Author: Tobias Koenig <tokoe@kde.org>

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

#include "emailaddressselectionwidgettest.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcombobox.h>
#include <kcmdlineargs.h>
#include <ktextbrowser.h>
#include <klocale.h>

#include <QGridLayout>
#include <QPushButton>
#include <QTreeView>

MainWidget::MainWidget()
  : QWidget( 0 )
{
  QGridLayout *layout = new QGridLayout( this );

  mAddressesWidget = new Akonadi::EmailAddressSelectionWidget;
  layout->addWidget( mAddressesWidget, 0, 0 );

  mInfo = new KTextBrowser;
  layout->addWidget( mInfo, 0, 1 );

  KComboBox *box = new KComboBox;
  box->addItem( QLatin1String( "Single Selection" ) );
  box->addItem( QLatin1String( "Multi Selection" ) );
  connect( box, SIGNAL(activated(int)), SLOT(selectionModeChanged(int)) );
  layout->addWidget( box, 1, 0 );

  QPushButton *button = new QPushButton( QLatin1String( "Show Selection" ) );
  connect( button, SIGNAL(clicked()), SLOT(showSelection()) );
  layout->addWidget( button, 1, 1 );
}

void MainWidget::selectionModeChanged( int index )
{
  mAddressesWidget->view()->setSelectionMode( index == 0 ? QTreeView::SingleSelection : QTreeView::MultiSelection );
}

void MainWidget::showSelection()
{
  mInfo->append( QLatin1String( "===========================\n" ) );
  mInfo->append( QLatin1String( "Current selection:\n" ) );

  foreach ( const Akonadi::EmailAddressSelection &selection, mAddressesWidget->selectedAddresses() ) {
    mInfo->append( QString::fromLatin1( "%1: %2\n" ).arg( selection.name() ).arg( selection.email() ) );
  }
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "emailaddressselectionwidgettest", 0, ki18n( "Test EmailAddressSelectionWidget" ), "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  MainWidget wdg;
  wdg.show();

  return app.exec();
}
