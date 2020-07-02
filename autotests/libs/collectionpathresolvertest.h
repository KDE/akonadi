/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef COLLECTIONPATHRESOLVER_TEST_H
#define COLLECTIONPATHRESOLVER_TEST_H

#include <QObject>

class CollectionPathResolverTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testPathResolver();
    void testRoot();
    void testFailure();
};

#endif
