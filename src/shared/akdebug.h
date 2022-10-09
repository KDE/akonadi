/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QString>

/**
 * Init and rotate error logs.
 */
void akInit(const QString &appName);

void akMakeVerbose(const QByteArray &category);
