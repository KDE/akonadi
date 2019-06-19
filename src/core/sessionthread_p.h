/*
    Copyright (c) 2016 Daniel Vrátil <dvratil@kde.org>

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

#ifndef SESSIONTHREAD_P_H
#define SESSIONTHREAD_P_H

#include <QObject>
#include <QVector>

#include "connection_p.h"

class QEventLoop;

namespace Akonadi
{
class CommandBuffer;
class SessionThread : public QObject
{
    Q_OBJECT

public:
    explicit SessionThread(QObject *parent = nullptr);
    ~SessionThread();

    void addConnection(Connection *connection);
    void destroyConnection(Connection *connection);

private Q_SLOTS:
    void doDestroyConnection(Akonadi::Connection *connection);
    void doAddConnection(Akonadi::Connection *connection);

    void doThreadQuit();

private:
    QVector<Connection *> mConnections;
};

}

#endif
