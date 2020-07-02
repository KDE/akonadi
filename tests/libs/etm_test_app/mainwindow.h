/*
    SPDX-FileCopyrightText: 2010 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Akonadi
{
class EntityTreeModel;
}

class FakeServerData;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr, Qt::WindowFlags flags = {});

private Q_SLOTS:
    void moveCollection();

private:
    FakeServerData *m_serverData = nullptr;
    Akonadi::EntityTreeModel *m_model = nullptr;
};

#endif
