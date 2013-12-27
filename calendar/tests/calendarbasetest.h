/*
    Copyright (c) 2010-2011 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef CALENDARBASE_TEST_H_
#define CALENDARBASE_TEST_H_

#include <akonadi/collection.h>
#include <akonadi/item.h>


#include <QTestEventLoop>
#include <QtCore/QObject>
#include <QStringList>

namespace Akonadi {
class CalendarBase;
}

class CalendarBaseTest : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void handleCreateFinished(bool success, const QString &errorString);
    void handleDeleteFinished(bool success, const QString &errorString);

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testItem();
    void testChildIncidences_data();
    void testChildIncidences();
    void testDelete();
    // void testDeleteAll(); This has been disabled in KCalCore::Calendar::deleteAll*() so no need to test

private:
    void fetchCollection();
    void createInitialIncidences();
    Akonadi::Item::Id createTodo(const QString &uid, const QString &parentUid = QString());

    Akonadi::Collection mCollection;
    Akonadi::CalendarBase *mCalendar;
    bool mExpectedSlotResult;
    QStringList mUids;
    QString mOneEventUid;
    QString mOneTodoUid;
    QString mOneJournalUid;
    QString mOneIncidenceUid;

};

#endif
