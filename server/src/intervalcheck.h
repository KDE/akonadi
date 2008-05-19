/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef INTERVALCHECK_H
#define INTERVALCHECK_H

#include <QDateTime>
#include <QHash>
#include <QThread>

/**
  Interval checking thread.
*/
class IntervalCheck : public QThread
{
  Q_OBJECT

  public:
    IntervalCheck( QObject *parent = 0 );
    ~IntervalCheck();

  protected:
    void run();

  private slots:
    void doIntervalCheck();

  private:
    QHash<int, QDateTime> mLastChecks;
};

#endif
