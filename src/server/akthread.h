/*
    Copyright (c) 2015 Daniel Vrátil <dvratil@kde.org>

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

#ifndef AKONADI_SERVER_AKTHREAD_H
#define AKONADI_SERVER_AKTHREAD_H

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

    explicit AkThread(const QString &objectName, QThread::Priority priority = QThread::InheritPriority,
                      QObject *parent = nullptr);
    explicit AkThread(const QString &objectName, StartMode startMode,
                      QThread::Priority priority = QThread::InheritPriority,
                      QObject *parent = nullptr);
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

#endif // AKONADI_SERVER_AKTHREAD_H
