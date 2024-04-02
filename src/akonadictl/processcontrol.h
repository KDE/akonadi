/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDBusServiceWatcher>
#include <QObject>

#include "control.h"

class ProcessControl : public AbstractControl
{
    Q_OBJECT
public:
    explicit ProcessControl(QObject *parent = nullptr);
    [[nodiscard]] bool start(bool verbose) override;
    [[nodiscard]] bool stop() override;
    [[nodiscard]] Status status() const override;

private:
    const QDBusServiceWatcher mWatcher;
    bool mRegistered = false;
};
