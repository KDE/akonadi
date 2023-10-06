/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QDBusServiceWatcher>
#include <QObject>

class AkonadiStarter : public QObject
{
    Q_OBJECT
public:
    explicit AkonadiStarter(QObject *parent = nullptr);
    [[nodiscard]] bool start(bool verbose);

private:
    const QDBusServiceWatcher mWatcher;
    bool mRegistered = false;
};
