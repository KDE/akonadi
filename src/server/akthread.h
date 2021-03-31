/*
    SPDX-FileCopyrightText: 2015 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>
#include <QThread>

namespace Akonadi
{
namespace Server
{
class AkThread : public QObject
{
    Q_OBJECT
public:
    enum StartMode {
        AutoStart,
        ManualStart,
        NoThread // for unit-tests
    };

    explicit AkThread(const QString &objectName, QThread::Priority priority = QThread::InheritPriority, QObject *parent = nullptr);
    explicit AkThread(const QString &objectName, StartMode startMode, QThread::Priority priority = QThread::InheritPriority, QObject *parent = nullptr);
    ~AkThread() override;

protected:
    void quitThread();
    void startThread();

protected Q_SLOTS:
    virtual void init();
    virtual void quit();

private:
    StartMode m_startMode = AutoStart;
};

}
}

