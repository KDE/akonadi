/*
    SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "shared/akremotelog.h"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>

namespace
{
const auto initRemoteLogger = []() {
    qAddPreRoutine([]() {
        // Initialize remote logging from event loop, this way applications like
        // Akonadi Console or TestRunner have a chance to change AKONADI_INSTANCE
        // before the RemoteLog class initialize
        QTimer::singleShot(0, qApp, []() {
            akInitRemoteLog();
        });
    });
    return true;
}();

} // namespace
