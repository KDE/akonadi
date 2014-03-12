/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef RACETEST_H
#define RACETEST_H

#include <QtCore/QObject>
#include <QList>

class KProcess;

/**
  This tests the ability of LocalFolders to exist peacefully in multiple processes.
  The main instance (normally the first one created) is supposed to create the
  resource and collections, while the other instances are supposed to wait and
  then just fetch the collections.
 */
class RaceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMultipleProcesses_data();
    void testMultipleProcesses();
    void killZombies();

private:
    QList<KProcess *> procs;

};

#endif
