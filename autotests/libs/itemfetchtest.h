/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class ItemFetchTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testFetch();
    void testResourceRetrieval();
    void testIllegalFetch();
    void testMultipartFetch_data();
    void testMultipartFetch();
    void testRidFetch();
    void testAncestorRetrieval();
};
