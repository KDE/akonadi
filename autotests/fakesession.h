/*
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

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

#ifndef FAKESESSION_H
#define FAKESESSION_H

#include "session.h"
#include "collection.h"

using namespace Akonadi;

class FakeSession : public Session
{
  Q_OBJECT
public:
  enum Mode {
    EndJobsImmediately,
    EndJobsManually
  };

  explicit FakeSession(const QByteArray& sessionId = QByteArray(), Mode mode = EndJobsImmediately, QObject* parent = 0);

  /** Make this the default session returned by Akonadi::Session::defaultSession().
   *  Note that ownership is taken over by the thread-local storage.
   */
  void setAsDefaultSession();

Q_SIGNALS:
  void jobAdded( Akonadi::Job* );

  friend class FakeSessionPrivate;
};

#endif
