/*
    SPDX-FileCopyrightText: 2016 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "aklocalserver.h"

using namespace Akonadi::Server;

AkLocalServer::AkLocalServer(QObject *parent)
    : QLocalServer(parent)
{
}

void AkLocalServer::incomingConnection(quintptr socketDescriptor)
{
    Q_EMIT newConnection(socketDescriptor);
}

#include "moc_aklocalserver.cpp"
