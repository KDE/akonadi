/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef SESSIONTHREAD_P_H
#define SESSIONTHREAD_P_H

#include <QObject>
#include <QVector>

#include "connection_p.h"


namespace Akonadi
{
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
