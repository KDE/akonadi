/*
    Copyright (c) 2010 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_KJOBPRIVATEBASE_P_H
#define AKONADI_KJOBPRIVATEBASE_P_H

#include <QtCore/QObject>

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
