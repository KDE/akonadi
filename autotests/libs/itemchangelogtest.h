/*
    SPDX-FileCopyrightText: 2023 Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ItemChangelogTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDoesntCrashWhenReassigning();
};
