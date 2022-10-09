/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ImapSetTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testAddList_data();
    void testAddList();
};
