/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "kjobprivatebase_p.h"

using namespace Akonadi;

void KJobPrivateBase::start()
{
    const ServerManager::State serverState = ServerManager::state();

    if (serverState == ServerManager::Running) {
        doStart();
        return;
    }

    connect(ServerManager::self(), &ServerManager::stateChanged, this, &KJobPrivateBase::serverStateChanged);

    if (serverState == ServerManager::NotRunning) {
        ServerManager::start();
    }
}

void KJobPrivateBase::serverStateChanged(Akonadi::ServerManager::State state)
{
    if (state == ServerManager::Running) {
        disconnect(ServerManager::self(), &ServerManager::stateChanged, this, &KJobPrivateBase::serverStateChanged);
        doStart();
    }
}

#include "moc_kjobprivatebase_p.cpp"
