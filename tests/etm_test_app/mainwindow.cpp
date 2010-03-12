/*
    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

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

#include "mainwindow.h"

#include <akonadi/control.h>

#include "fakeserverdata.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "public_etm.h"
#include <Akonadi/EntityTreeView>

using namespace Akonadi;

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags)
  : QMainWindow(parent, flags)
{
  FakeMonitor *monitor = new FakeMonitor( this );
  FakeSession *session = new FakeSession( "FS1", this );
  monitor->setSession( session );

  PublicETM *model = new PublicETM( monitor, this );
  FakeServerData *serverData = new FakeServerData( model, session, monitor );
  serverData->interpret(
    "- C (inode/directory) 4"
    "- - C (text/directory, message/rfc822) 3"
    "- - - I text/directory"
    "- - - I text/directory"
    "- - - I message/rfc822"
    "- - - I message/rfc822"
    "- - C (text/directory) 3"
    "- - - C (text/directory) 2"
    "- - - - C (text/directory) 1"
    "- - - - - I text/directory"
    "- - - - - I text/directory"
    "- - - I text/directory"
    "- - - I text/directory"
    "- - C (message/rfc822) 3"
    "- - - I message/rfc822"
    "- - - I message/rfc822"
    "- - C (text/directory, message/rfc822) 3"
    "- - - I text/directory"
    "- - - I text/directory"
    "- - - I message/rfc822"
    "- - - I message/rfc822"
  );

  EntityTreeView *view = new EntityTreeView( this );
  view->setModel( model );

  setCentralWidget( view );
}

#include "mainwindow.moc"
