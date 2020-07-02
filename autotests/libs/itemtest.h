/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_ITEMTEST_H
#define AKONADI_ITEMTEST_H

#include <QObject>

class ItemTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testMultipart();
    void testInheritance();
    void testParentCollection();

    void testComparison_data();
    void testComparison();
};

#endif
