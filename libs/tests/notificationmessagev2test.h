/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>
    Copyright (c) 2013 Daniel Vrátil <dvratil@redhat.com>

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

#ifndef AKONADI_NOTIFICATIONMESSAGEV2TEST_H
#define AKONADI_NOTIFICATIONMESSAGEV2TEST_H

#include <QtCore/QObject>

class NotificationMessageV2Test : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testCompress();
    void testCompress2();
    void testCompress3();
    void testCompress4();
    void testCompress5();
    void testCompress6();
    void testCompress7();
    void testCompressWithItemParts();
    void testNoCompress();
    void testPartModificationMerge_data();
    void testPartModificationMerge();
};

#endif
