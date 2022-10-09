/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

class DbUpdaterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testMysqlUpdateStatements();
    void testPsqlUpdateStatements();
    void cleanupTestCase();
};
