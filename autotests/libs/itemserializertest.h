/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ITEMSERIALIZERTEST_H
#define ITEMSERIALIZERTEST_H

#include <QObject>

class ItemSerializerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testEmptyPayload();
    void testDefaultSerializer_data();
    void testDefaultSerializer();
};

#endif
