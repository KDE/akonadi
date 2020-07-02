/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef COLLECTIONJOBTEST_H
#define COLLECTIONJOBTEST_H

#include <QObject>

class CollectionJobTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void testTopLevelList();
    void testFolderList();
    void testSignalOrder();
    void testNonRecursiveFolderList();
    void testEmptyFolderList();
    void testSearchFolderList();
    void testResourceFolderList();
    void testMimeTypeFilter();
    void testCreateDeleteFolder_data();
    void testCreateDeleteFolder();
    void testIllegalDeleteFolder();
    void testStatistics();
    void testModify_data();
    void testModify();
    void testIllegalModify();
    void testUtf8CollectionName_data();
    void testUtf8CollectionName();
    void testMultiList();
    void testMultiListInvalid();
    void testRecursiveMultiList();
    void testNonOverlappingRootList();
    void testRidFetch();
    void testRidCreateDelete_data();
    void testRidCreateDelete();
    void testAncestorRetrieval();
    void testAncestorAttributeRetrieval();
    void testListPreference();
};

#endif
