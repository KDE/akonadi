/*
    SPDX-FileCopyrightText: 2008 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef TRANSACTIONTEST_H
#define TRANSACTIONTEST_H

#include <QObject>

class TransactionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testTransaction();
};

#endif
