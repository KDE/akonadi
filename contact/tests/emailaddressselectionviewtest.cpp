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

#include "emailaddressselectionviewtest.h"

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>

#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QPushButton>
#include <QtGui/QTextBrowser>
#include <QtGui/QTreeView>

MainWidget::MainWidget()
  : QWidget( 0 )
{
  QGridLayout *layout = new QGridLayout( this );

  mAddressesView = new Akonadi::EmailAddressSelectionView;
  layout->addWidget( mAddressesView, 0, 0 );

  mInfo = new QTextBrowser;
  layout->addWidget( mInfo, 0, 1 );

  QComboBox *box = new QComboBox;
  box->addItem( QLatin1String( "Single Selection" ) );
  box->addItem( QLatin1String( "Multi Selection" ) );
  connect( box, SIGNAL( activated( int ) ), SLOT( selectionModeChanged( int ) ) );
  layout->addWidget( box, 1, 0 );

  QPushButton *button = new QPushButton( QLatin1String( "Show Selection" ) );
  connect( button, SIGNAL( clicked() ), SLOT( showSelection() ) );
  layout->addWidget( button, 1, 1 );
}

void MainWidget::selectionModeChanged( int index )
{
  mAddressesView->setSelectionMode( index == 0 ? QTreeView::SingleSelection : QTreeView::MultiSelection );
}

void MainWidget::showSelection()
{
  mInfo->append( QLatin1String( "===========================\n" ) );
  mInfo->append( QLatin1String( "Current selection:\n" ) );

  const Akonadi::EmailAddressSelectionView::Selection::List selections = mAddressesView->selectedAddresses();
  foreach ( const Akonadi::EmailAddressSelectionView::Selection &selection, selections )
    mInfo->append( QString::fromLatin1( "%1: %2\n" ).arg( selection.name() ).arg( selection.email() ) );
}

int main( int argc, char **argv )
{
  KAboutData aboutData( "emailaddressselectionviewtest", 0, ki18n( "Test EmailAddressSelectionView"), "0.1" );
  KCmdLineArgs::init( argc, argv, &aboutData );

  KApplication app;

  MainWidget wdg;
  wdg.show();

  return app.exec();
}
