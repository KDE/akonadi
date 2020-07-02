/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2007 Robert Zwerus <arzie@dds.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef ITEMSTORETEST_H
#define ITEMSTORETEST_H

#include <QObject>

class ItemStoreTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testFlagChange();
    void testDataChange_data();
    void testDataChange();
    void testRemoteId_data();
    void testRemoteId();
    void testMultiPart();
    void testPartRemove();
    void testRevisionCheck();
    void testModificationTime();
    void testRemoteIdRace();
    void itemModifyJobShouldOnlySendModifiedAttributes();
    void testParallelJobsAddingAttributes();
};

#endif
