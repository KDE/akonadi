/*
    Copyright (c) 2010 Bertjan Broeksema <broeksema@kde.org>

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
#ifndef UIRUNNABLE_H
#define UIRUNNABLE_H

namespace Akonadi {

/**
  Agents need to perform tasks that need to be done in the gui thread, e.g.
  creation of widgets. When running in their own processes this is no problem.
  However, when running multiple agents in different threads of the same process
  this does become a problem. This class is an attempt to overcome that problem.

  Agents and Resources can subclass this interface (but should <em>never</em>
  make it a subclass from QObject as well) and emit the
  runRequest( UiRunnable * ) signal. The AgentServer connects to this signal for
  each Agent instance in a blocked and queued way (i.e. it uses
  Qt::BlockingQueuedConnection).

  After the signal has been emitted relevant information can be retrieved from
  the subclass.
 */
class UiRunnable
{
public:
  virtual ~UiRunnable() {}

  virtual void run() = 0;
};

}

#endif // UIRUNNABLE_H
