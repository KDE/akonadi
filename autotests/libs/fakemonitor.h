/*
    SPDX-FileCopyrightText: 2009 Stephen Kelly <steveire@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef FAKEMONITOR_H
#define FAKEMONITOR_H

#include "akonaditestfake_export.h"
#include "changerecorder.h"

using namespace Akonadi;

class FakeMonitorPrivate;

class AKONADITESTFAKE_EXPORT FakeMonitor : public Akonadi::ChangeRecorder
{
    Q_OBJECT
public:
    explicit FakeMonitor(QObject *parent = nullptr);

private:
    Q_DECLARE_PRIVATE(FakeMonitor)
};

#endif
