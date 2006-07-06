/***************************************************************************
 *   Copyright (C) 2006 by Tobias Koenig <tokoe@kde.org>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QtGui/QSplitter>
#include <QtGui/QTabWidget>
#include <QtGui/QTextEdit>

#include "mainwindow.h"

#include "tracernotificationinterface.h"
#include "connectionpage.h"

MainWindow::MainWindow()
  : QMainWindow( 0 )
{
  QSplitter *splitter = new QSplitter( Qt::Vertical, this );
  setCentralWidget( splitter );

  mConnectionPages = new QTabWidget( splitter );

  mGeneralView = new QTextEdit( splitter );
  mGeneralView->setReadOnly( true );

  ConnectionPage *page = new ConnectionPage( "All" );
  page->showAllConnections( true );
  mConnectionPages->addTab( page, "All" );

  org::kde::Akonadi::TracerNotification *iface = new org::kde::Akonadi::TracerNotification( QString(), "/tracing", QDBus::sessionBus(), this );
  connect( iface, SIGNAL( connectionStarted( const QString&, const QString& ) ),
           this, SLOT( connectionStarted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( connectionEnded( const QString&, const QString& ) ),
           this, SLOT( connectionEnded( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( signalEmitted( const QString&, const QString& ) ),
           this, SLOT( signalEmitted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( warningEmitted( const QString&, const QString& ) ),
           this, SLOT( warningEmitted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( errorEmitted( const QString&, const QString& ) ),
           this, SLOT( errorEmitted( const QString&, const QString& ) ) );
}

void MainWindow::connectionStarted( const QString &identifier, const QString &msg )
{
  ConnectionPage *page = new ConnectionPage( identifier );
  mConnectionPages->addTab( page, msg );

  mPageHash.insert( identifier, page );
}

void MainWindow::connectionEnded( const QString &identifier, const QString &msg )
{
  if ( !mPageHash.contains( identifier ) )
    return;

  QWidget *widget = mPageHash[ identifier ];

  mConnectionPages->removeTab( mConnectionPages->indexOf( widget ) );

  mPageHash.remove( identifier );
  delete widget;
}

void MainWindow::signalEmitted( const QString &signalName, const QString &msg )
{
  mGeneralView->append( QString( "<font color=\"green\">%1 ( %2 )</font>" ).arg( signalName, msg ) );
}

void MainWindow::warningEmitted( const QString &componentName, const QString &msg )
{
  mGeneralView->append( QString( "<font color=\"blue\">%1: %2</font>" ).arg( componentName, msg ) );
}

void MainWindow::errorEmitted( const QString &componentName, const QString &msg )
{
  mGeneralView->append( QString( "<font color=\"red\">%1: %2</font>" ).arg( componentName, msg ) );
}

#include "mainwindow.moc"
