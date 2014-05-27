/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef COLLECTIONJOBTEST_H
#define COLLECTIONJOBTEST_H

#include <QtCore/QObject>

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
    void testRecursiveMultiList();
    void testNonOverlappingRootList();
    void testSelect();
    void testRidFetch();
    void testRidCreateDelete_data();
    void testRidCreateDelete();
    void testAncestorRetrieval();
};

#endif
