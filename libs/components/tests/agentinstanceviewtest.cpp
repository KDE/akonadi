/*
    Copyright (c) 2006 Tobias Koenig <tokoe@kde.org>

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

#include <QtDBus/QDBusConnection>
#include <QtGui/QApplication>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>

#include "agentinstanceviewtest.h"

Dialog::Dialog( QWidget *parent )
  : QDialog( parent )
{
  QVBoxLayout *layout = new QVBoxLayout( this );

  mView = new PIM::AgentInstanceView( this );
  connect( mView, SIGNAL( currentChanged( const QString&, const QString& ) ),
           this, SLOT( currentChanged( const QString&, const QString& ) ) );

  QDialogButtonBox *box = new QDialogButtonBox( this );

  layout->addWidget( mView );
  layout->addWidget( box );

  QPushButton *ok = box->addButton( QDialogButtonBox::Ok );
  connect( ok, SIGNAL( clicked() ), this, SLOT( accept() ) );

  resize( 450, 320 );
}

void Dialog::done( int r )
{
  if ( r == Accepted ) {
    qDebug( "'%s' selected", qPrintable( mView->currentAgentInstance() ) );
  }

  QDialog::done( r );
}

void Dialog::currentChanged( const QString &current, const QString &previous )
{
  qDebug( "current changed: %s -> %s", qPrintable( previous ), qPrintable( current ) );
}

int main( int argc, char **argv )
{
  QApplication app( argc, argv );

  if ( !QDBusConnection::sessionBus().registerService( "org.kde.Test.agentinstanceviewtest" ) ) {
    qDebug( "Unable to register service at dbus" );
    return 1;
  }

  Dialog dlg;
  dlg.exec();

  return 0;
};

#include "agentinstanceviewtest.moc"
