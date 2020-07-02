/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKDEBUG_H
#define AKDEBUG_H

#include <QString>

/**
 * Init and rotate error logs.
 */
void akInit(const QString &appName);

void akMakeVerbose(const QByteArray &category);

#endif
