/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ItemAppendTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testItemAppend_data();
    void testItemAppend();
    void testContent_data();
    void testContent();
    void testNewMimetype();
    void testIllegalAppend();
    void testMultipartAppend();
    void testInvalidMultipartAppend();
    void testItemSize_data();
    void testItemSize();
    void testItemMerge_data();
    void testItemMerge();
    void testForeignPayload();
};

