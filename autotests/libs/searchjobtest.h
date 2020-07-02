/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_SEARCHJOBTEST_H
#define AKONADI_SEARCHJOBTEST_H

#include <QObject>

class SearchJobTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testCreateDeleteSearch();
    void testModifySearch();
};

#endif
