/*
    SPDX-FileCopyrightText: 2009 Constantin Berzan <exit3219@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class AttributeFactoryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testUnknownAttribute();
    void testRegisteredAttribute();
};

