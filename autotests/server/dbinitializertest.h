/*
    SPDX-FileCopyrightText: 2010 Tobias Koenig <tokoe@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DBINITIALIZERTEST_H
#define DBINITIALIZERTEST_H

#include <QObject>
class QIODevice;
class DbInitializerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();

    void testRun_data();
    void testRun();

private:
    static QString readNextStatement(QIODevice *io);
};

#endif
