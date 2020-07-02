/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADISTARTER_H
#define AKONADISTARTER_H

#include <QObject>
#include <QDBusServiceWatcher>

class AkonadiStarter : public QObject
{
    Q_OBJECT
public:
    explicit AkonadiStarter(QObject *parent = nullptr);
    Q_REQUIRED_RESULT bool start(bool verbose);

private:
    QDBusServiceWatcher mWatcher;
    bool mRegistered = false;
};

#endif
