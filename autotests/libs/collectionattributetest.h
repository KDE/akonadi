/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class CollectionAttributeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testAttributes_data();
    void testAttributes();
    void testDefaultAttributes();
    void testCollectionRightsAttribute();
    void testCollectionIdentificationAttribute();
    void testDetach();
};

