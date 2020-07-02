/*
    SPDX-FileCopyrightText: 2010 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_KJOBPRIVATEBASE_P_H
#define AKONADI_KJOBPRIVATEBASE_P_H

#include <QObject>

#include "servermanager.h"

namespace Akonadi
{

/**
 * Base class for the private class of KJob but not Akonadi::Job based jobs that
 * require the Akonadi server to be operational.
 * Delays job execution until that is the case.
 * @internal
 */
class KJobPrivateBase : public QObject
{
    Q_OBJECT

public:
    /** Call from KJob::start() reimplementation. */
    void start();

    /** Reimplement and put here what was in KJob::start() before. */
    virtual void doStart() = 0;

private Q_SLOTS:
    void serverStateChanged(Akonadi::ServerManager::State state);
};

}

#endif
