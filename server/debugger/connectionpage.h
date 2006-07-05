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

#ifndef CONNECTIONPAGE_H
#define CONNECTIONPAGE_H

#include <QtGui/QWidget>

class QTextEdit;

class ConnectionPage : public QWidget
{
  Q_OBJECT

  public:
    ConnectionPage( const QString &identifier, QWidget *parent = 0 );

    void showAllConnections( bool );

  private Q_SLOTS:
    void connectionDataInput( const QString&, const QString& );
    void connectionDataOutput( const QString&, const QString& );

  private:
    QTextEdit *mDataView;
    QString mIdentifier;
    bool mShowAllConnections;
};

#endif
