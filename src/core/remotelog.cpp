/*
    Copyright (c) 2018 Daniel Vrátil <dvratil@kde.org>

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

#include <shared/akremotelog.h>

#include <QCoreApplication>
#include <QThread>
#include <QTimer>

namespace {

const auto initRemoteLogger = []() {
    qAddPreRoutine([]() {
        // Initialize remote logging from event loop, this way applications like
        // Akonadi Console or TestRunner have a chance to change AKONADI_INSTANCE
        // before the RemoteLog class initialize
        QTimer::singleShot(0, qApp, []() { akInitRemoteLog(); });
    });
    return true;
}();

} // namespace
