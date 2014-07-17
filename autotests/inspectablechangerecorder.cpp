/*
    Copyright (c) 2011 Stephen Kelly <steveire@gmail.com>

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

#include "inspectablechangerecorder.h"

#include "changerecorder_p.h"

InspectableChangeRecorderPrivate::InspectableChangeRecorderPrivate(FakeMonitorDependeciesFactory* dependenciesFactory, InspectableChangeRecorder* parent)
  : ChangeRecorderPrivate(dependenciesFactory, parent)
{

}

InspectableChangeRecorder::InspectableChangeRecorder(FakeMonitorDependeciesFactory* dependenciesFactory, QObject* parent)
  : ChangeRecorder(new Akonadi::ChangeRecorderPrivate(dependenciesFactory, this), parent)
{
  QTimer::singleShot( 0, this, SLOT(doConnectToNotificationManager()) );
}

void InspectableChangeRecorder::doConnectToNotificationManager()
{
  d_ptr->connectToNotificationManager();
}

