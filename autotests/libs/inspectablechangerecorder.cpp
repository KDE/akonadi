/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "inspectablechangerecorder.h"
using namespace std::chrono_literals;
InspectableChangeRecorderPrivate::InspectableChangeRecorderPrivate(FakeMonitorDependenciesFactory *dependenciesFactory, InspectableChangeRecorder *parent)
    : ChangeRecorderPrivate(dependenciesFactory, parent)
{
}

InspectableChangeRecorder::InspectableChangeRecorder(FakeMonitorDependenciesFactory *dependenciesFactory, QObject *parent)
    : ChangeRecorder(new Akonadi::ChangeRecorderPrivate(dependenciesFactory, this), parent)
{
    QTimer::singleShot(0s, this, &InspectableChangeRecorder::doConnectToNotificationManager);
}

void InspectableChangeRecorder::doConnectToNotificationManager()
{
    d_ptr->connectToNotificationManager();
}

#include "moc_inspectablechangerecorder.cpp"
