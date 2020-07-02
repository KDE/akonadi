/*
    SPDX-FileCopyrightText: 2007 Till Adam <adam@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ITEMHYDRA_H
#define ITEMHYDRA_H

#include <QObject>

class ItemHydra : public QObject
{
    Q_OBJECT
public:
    ItemHydra();
    virtual ~ItemHydra()
    {
    }
private Q_SLOTS:
    void initTestCase();
    void testItemValuePayload();
    void testItemPointerPayload();
    void testItemCopy();
    void testEmptyPayload();
    void testPointerPayload();
    void testPolymorphicPayloadWithTrait();
    void testPolymorphicPayloadWithTypedef();
    void testNullPointerPayload();
    void testQSharedPointerPayload();
    void testHasPayload();
    void testSharedPointerConversions();
    void testForeignPayload();
};

#endif
