/*
    SPDX-FileCopyrightText: 2009 Thomas McGuire <mcguire@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#pragma once

#include <QObject>
#include <QVariant>

class ResourceSchedulerTest : public QObject
{
    Q_OBJECT
public:
    explicit ResourceSchedulerTest(QObject *parent = nullptr);

public Q_SLOTS:
    void customTask(const QVariant &argument);
    void customTaskNoArg();

private Q_SLOTS:

    void testTaskComparison();
    void testChangeReplaySchedule();
    void testCustomTask();
    void testCompression();
    void testSyncCompletion();
    void testPriorities();

private:
    int mCustomCallCount;
    QVariant mLastArgument;
};

