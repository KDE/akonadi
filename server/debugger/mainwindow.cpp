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

#include "tracerinterface.h"
#include "connectionpage.h"

MainWindow::MainWindow()
  : QMainWindow( 0 )
{
  QSplitter *splitter = new QSplitter( Qt::Vertical, this );
  setCentralWidget( splitter );

  mConnectionPages = new QTabWidget( splitter );

  mSignalsView = new QTextEdit( splitter );
  mSignalsView->setReadOnly( true );

  org::kde::Akonadi::Tracer *iface = new org::kde::Akonadi::Tracer( QString(), "/tracing", QDBus::sessionBus(), this );
  connect( iface, SIGNAL( connectionStarted( const QString&, const QString& ) ),
           this, SLOT( connectionStarted( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( connectionEnded( const QString&, const QString& ) ),
           this, SLOT( connectionEnded( const QString&, const QString& ) ) );
  connect( iface, SIGNAL( dbusSignalEmitted( const QString&, const QString& ) ),
           this, SLOT( dbusSignalEmitted( const QString&, const QString& ) ) );
}

void MainWindow::connectionStarted( const QString &identifier, const QString &msg )
{
  ConnectionPage *page = new ConnectionPage( identifier, mConnectionPages );
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

void MainWindow::dbusSignalEmitted( const QString &signalName, const QString &msg )
{
  mSignalsView->append( QString( "<font color=\"green\">%1 ( %2 )</font>" ).arg( signalName, msg ) );
}

#include "mainwindow.moc"
