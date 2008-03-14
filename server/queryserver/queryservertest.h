/***************************************************************************
 *   Copyright (C) 2008 by Tobias Koenig <tokoe@kde.org>                   *
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

#ifndef AKONADI_QUERYSERVERTEST_H
#define AKONADI_QUERYSERVERTEST_H

#include <QtCore/QObject>

#include "searchinterface.h"
#include "searchqueryinterface.h"

class TestObject : public QObject
{
  Q_OBJECT

  public:
    TestObject( const QString &query, QObject *parent = 0 );

  private Q_SLOTS:
    void hitsChanged( const QMap<QString, double> &hits );
    void hitsRemoved( const QMap<QString, double> &hits );

  private:
    org::kde::Akonadi::Search *mSearch;
    org::kde::Akonadi::SearchQuery *mQuery;
};

#endif
