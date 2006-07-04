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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore/QHash>
#include <QtGui/QMainWindow>

class QTextEdit;
class QTabWidget;

class ConnectionPage;

class MainWindow : public QMainWindow
{
  Q_OBJECT

  public:
    MainWindow();

  private Q_SLOTS:
    void connectionStarted( const QString&, const QString& );
    void connectionEnded( const QString&, const QString& );
    void dbusSignalEmitted( const QString&, const QString& );

  private:
    QTextEdit *mSignalsView;
    QTabWidget *mConnectionPages;
    QHash<QString, ConnectionPage*> mPageHash;
};

#endif
