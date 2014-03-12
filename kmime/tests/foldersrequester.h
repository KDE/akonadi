/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#ifndef FOLDERSREQUESTER_H
#define FOLDERSREQUESTER_H

#include <QObject>

class KJob;

/**
  This class requests some LocalFolders, then exits the app with a status of 2
  if they were delivered OK, or 1 if they were not.

  NOTE: The non-standard exit status 2 in case of success is to make feel more
  comfortable than checking for zero (I actually had a bug causing it to always
  return zero).
*/
class Requester : public QObject
{
    Q_OBJECT

public:
    Requester();

private Q_SLOTS:
    void requestResult(KJob *job);
};

#endif
