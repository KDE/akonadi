/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class MonitorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testMonitor_data();
    void testMonitor();
    void testVirtualCollectionsMonitoring();
};
