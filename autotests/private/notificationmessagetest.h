/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class NotificationMessageTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCompress();
    void testCompress2();
    void testCompress3();
    void testPartModificationMerge();
};

