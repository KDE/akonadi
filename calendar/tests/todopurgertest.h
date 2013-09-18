/*
    Copyright (c) 2013 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef TODOPURGER_TEST_H_
#define TODOPURGER_TEST_H_

#include <kcalcore/calendar.h>
#include <akonadi/collection.h>
#include <QObject>
#include <QString>

namespace Akonadi {
    class ETMCalendar;
    class TodoPurger;
}

class TodoPurgerTest : public QObject, KCalCore::Calendar::CalendarObserver
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testPurge();

public:
    void calendarIncidenceAdded( const KCalCore::Incidence::Ptr &incidence );   /**Q_DECL_OVERRIDE*/
    void calendarIncidenceDeleted( const KCalCore::Incidence::Ptr &incidence ); /**Q_DECL_OVERRIDE*/

public Q_SLOTS:
    void onTodosPurged(bool success, int numDeleted, int numIgnored);

private:
    void createTree();
    void createTodo(const QString &uid, const QString &parentUid, bool completed);
    void fetchCollection();

    Akonadi::ETMCalendar *m_calendar;
    Akonadi::Collection m_collection;
    int m_pendingCreations;
    int m_pendingDeletions;
    bool m_pendingPurgeSignal;

    int m_numDeleted;
    int m_numIgnored;

    Akonadi::TodoPurger *m_todoPurger;
};

#endif
