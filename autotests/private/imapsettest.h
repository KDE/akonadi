/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef AKONADI_IMAPSETTEST_H
#define AKONADI_IMAPSETTEST_H

#include <QObject>

class ImapSetTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testAddList_data();
    void testAddList();
};

#endif
