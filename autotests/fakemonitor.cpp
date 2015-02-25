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

#include "fakemonitor.h"
#include "changerecorder_p.h"

#include "entitycache_p.h"

#include <QMetaMethod>

using namespace Akonadi;

class FakeMonitorPrivate : public ChangeRecorderPrivate
{
  Q_DECLARE_PUBLIC(FakeMonitor)
public:
  FakeMonitorPrivate( FakeMonitor *monitor )
    : ChangeRecorderPrivate( 0, monitor )
  {
  }

  bool connectToNotificationManager() Q_DECL_OVERRIDE
  {
    // Do nothing. This monitor should not connect to the notification manager.
    return true;
  }

};

FakeMonitor::FakeMonitor(QObject* parent)
  : ChangeRecorder( new FakeMonitorPrivate( this ), parent )
{

}
