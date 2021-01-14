/*
    SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "error.h"

#include <QDebug>

using namespace Akonadi;

QDebug operator<<(QDebug dbg, const Error &error)
{
    if (!error) {
        return dbg << "Akonadi::Error(0, \"No Error\")";
    }
    QDebugStateSaver saver(dbg);
    return dbg.nospace().noquote() << "Akonadi::Error(" << error.code() << ", \"" << error.message() << "\")";
}