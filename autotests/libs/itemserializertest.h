/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ItemSerializerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testEmptyPayload();
    void testDefaultSerializer_data();
    void testDefaultSerializer();
};
