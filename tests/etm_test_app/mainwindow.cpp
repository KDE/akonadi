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

#include "control.h"

#include "fakeserverdata.h"
#include "fakesession.h"
#include "fakemonitor.h"
#include "entitytreeview.h"
#include <QTimer>

using namespace Akonadi;

MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags)
    : QMainWindow(parent, flags)
{
    FakeMonitor *monitor = new FakeMonitor(this);
    FakeSession *session = new FakeSession("FS1", FakeSession::EndJobsImmediately, this);
    monitor->setSession(session);

    m_model = new EntityTreeModel(monitor, this);
    m_serverData = new FakeServerData(m_model, session, monitor);

    QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret(m_serverData, QLatin1String(
        "- C (inode/directory) 'Col 1' 4"
        "- - C (text/directory, message/rfc822) 'Col 2' 3"
        // Items just have the mimetype they contain in the payload.
        "- - - I text/directory"
        "- - - I text/directory 'Item 1'"
        "- - - I message/rfc822"
        "- - - I message/rfc822"
        "- - C (text/directory) 'Col 3' 3"
        "- - - C (text/directory) 'Col 4' 2"
        "- - - - C (text/directory) 'Col 5' 1"  // <-- First collection to be returned
        "- - - - - I text/directory"
        "- - - - - I text/directory"
        "- - - - I text/directory"
        "- - - I text/directory"
        "- - - I text/directory"
        "- - C (message/rfc822) 'Col 6' 3"
        "- - - I message/rfc822"
        "- - - I message/rfc822"
        "- - C (text/directory, message/rfc822) 'Col 7' 3"
        "- - - I text/directory"
        "- - - I text/directory"
        "- - - I message/rfc822"
        "- - - I message/rfc822"));
    m_serverData->setCommands(initialFetchResponse);

    EntityTreeView *view = new EntityTreeView(this);
    view->setModel(m_model);

    view->expandAll();
    setCentralWidget(view);

    QTimer::singleShot(5000, this, SLOT(moveCollection()));
}

void MainWindow::moveCollection()
{
    // Move Col 3 from Col 4 to Col 7
    FakeCollectionMovedCommand *moveCommand = new FakeCollectionMovedCommand(QLatin1String("Col 4"), QLatin1String("Col 3"), QLatin1String("Col 7"), m_serverData);

    m_serverData->setCommands(QList<FakeAkonadiServerCommand *>() << moveCommand);
    m_serverData->processNotifications();
}
