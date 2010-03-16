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

#include "akonadieventqueue.h"
#include "fakeserver.h"
#include <QMetaMethod>

using namespace Akonadi;

FakeMonitor::FakeMonitor(QObject* parent)
  : ChangeRecorder(parent)
{
  connectForwardingSignals();
}

void FakeMonitor::connectForwardingSignals()
{
  for (int methodIndex = 0; methodIndex < metaObject()->methodCount(); ++methodIndex)
  {
    QMetaMethod mm = metaObject()->method(methodIndex);
    if (mm.methodType() == QMetaMethod::Signal && QString(mm.signature()).startsWith("emit_"))
    {
      int monitorSignalIndex = metaObject()->indexOfSignal( QString ( mm.signature() ).remove( 0, 5 ).toAscii().data() );
      Q_ASSERT( monitorSignalIndex >= 0 );
      metaObject()->connect(this, methodIndex, this, monitorSignalIndex );
    }
  }
}

void FakeMonitor::processNextEvent()
{
}




