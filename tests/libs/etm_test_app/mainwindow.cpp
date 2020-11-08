/*
    SPDX-FileCopyrightText: 2010 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
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
    auto *monitor = new FakeMonitor(this);
    FakeSession *session = new FakeSession("FS1", FakeSession::EndJobsImmediately, this);
    monitor->setSession(session);

    m_model = new EntityTreeModel(monitor, this);
    m_serverData = new FakeServerData(m_model, session, monitor, this);

    QList<FakeAkonadiServerCommand *> initialFetchResponse =  FakeJobResponse::interpret(m_serverData, QStringLiteral(
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

    auto *view = new EntityTreeView(this);
    view->setModel(m_model);

    view->expandAll();
    setCentralWidget(view);

    QTimer::singleShot(5000, this, &MainWindow::moveCollection);
}

void MainWindow::moveCollection()
{
    // Move Col 3 from Col 4 to Col 7
    FakeCollectionMovedCommand *moveCommand = new FakeCollectionMovedCommand(QStringLiteral("Col 4"), QStringLiteral("Col 3"), QStringLiteral("Col 7"), m_serverData);

    m_serverData->setCommands(QList<FakeAkonadiServerCommand *>() << moveCommand);
    m_serverData->processNotifications();
}
