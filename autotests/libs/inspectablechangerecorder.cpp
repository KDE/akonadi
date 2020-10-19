/*
    SPDX-FileCopyrightText: 2011 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "inspectablechangerecorder.h"


InspectableChangeRecorder::InspectableChangeRecorder(QObject *parent)
    : ChangeRecorder(new Akonadi::ChangeRecorderPrivate(this), parent)
{
    QTimer::singleShot(0, this, &InspectableChangeRecorder::doConnectToNotificationManager);
}

void InspectableChangeRecorder::doConnectToNotificationManager()
{
    d_ptr->connectToNotificationManager();
}

