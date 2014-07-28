/*
    Copyright (c) 2006-2008 Tobias Koenig <tokoe@kde.org>

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

#include "agenttypewidgettest.h"

#include "agenttype.h"
#include <AkonadiCore/agentfilterproxymodel.h>

#include <kcombobox.h>
#include <kcomponentdata.h>

#include <QApplication>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

//krazy:excludeall=qclasses

Dialog::Dialog( QWidget *parent )
  : QDialog( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  mFilter = new KComboBox( this );
  mFilter->addItem( QLatin1String("None") );
  mFilter->addItem( QLatin1String("text/calendar") );
  mFilter->addItem( QLatin1String("text/directory") );
  mFilter->addItem( QLatin1String("message/rfc822") );
  connect( mFilter, SIGNAL(activated(int)),
           this, SLOT(filterChanged(int)) );

  mWidget = new Akonadi::AgentTypeWidget( this );
  connect( mWidget, SIGNAL(currentChanged(Akonadi::AgentType,Akonadi::AgentType)),
           this, SLOT(currentChanged(Akonadi::AgentType,Akonadi::AgentType)) );

  QDialogButtonBox *box = new QDialogButtonBox( this );

  layout->addWidget( mFilter );
  layout->addWidget( mWidget );
  layout->addWidget( box );

  QPushButton *ok = box->addButton( QDialogButtonBox::Ok );
  connect( ok, SIGNAL(clicked()), this, SLOT(accept()) );

  QPushButton *cancel = box->addButton( QDialogButtonBox::Cancel );
  connect( cancel, SIGNAL(clicked()), this, SLOT(reject()) );

  resize( 450, 320 );
}

void Dialog::done( int r )
{
  if ( r == Accepted ) {
    qDebug( "'%s' selected", qPrintable( mWidget->currentAgentType().identifier() ) );
  }

  QDialog::done( r );
}

void Dialog::currentChanged( const Akonadi::AgentType &current, const Akonadi::AgentType &previous )
{
  qDebug( "current changed: %s -> %s", qPrintable( previous.identifier() ), qPrintable( current.identifier() ) );
}

void Dialog::filterChanged( int index )
{
  mWidget->agentFilterProxyModel()->clearFilters();
  if ( index > 0 )
    mWidget->agentFilterProxyModel()->addMimeTypeFilter( mFilter->itemText( index ) );
}

int main( int argc, char **argv )
{
  QApplication app( argc, argv );
  KComponentData kcd( "agenttypeviewtest" );

  Dialog dlg;
  dlg.exec();

  return 0;
}

