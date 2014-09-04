/*
    Copyright (c) 2011-2013 Sérgio Martins <iamsergio@gmail.com>

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

#ifndef ETMCALENDAR_TEST_H_
#define ETMCALENDAR_TEST_H_

#include <kcalcore/calendar.h>
#include <akonadi/collection.h>
#include <QObject>

namespace Akonadi {
class ETMCalendar;
}

class ETMCalendarTest : public QObject, KCalCore::Calendar::CalendarObserver
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void testCollectionChanged_data();
    void testCollectionChanged();
    void testIncidencesAdded();
    void testIncidencesModified();
    void testFilteredModel();
    void testUnfilteredModel();
    void testCheckableProxyModel();
    void testIncidencesDeleted();
    void testUnselectCollection();
    void testSelectCollection();
    void testSubTodos_data();
    void testSubTodos();
    void testNotifyObserverBug();
    void testUidChange();
    void testItem(); // tests item()
    void testShareETM();
    void testFilterInvitations();
    void testFilterInvitationsChanged();

public Q_SLOTS:
    void calendarIncidenceAdded(const KCalCore::Incidence::Ptr &incidence);   /**Q_DECL_OVERRIDE*/
    void calendarIncidenceChanged(const KCalCore::Incidence::Ptr &incidence); /**Q_DECL_OVERRIDE*/
    void calendarIncidenceDeleted(const KCalCore::Incidence::Ptr &incidence); /**Q_DECL_OVERRIDE*/
    void handleCollectionsAdded(const Akonadi::Collection::List &);

private:
    void deleteIncidence(const QString &uid);
    void createIncidence(const QString &uid);
    void createTodo(const QString &uid, const QString &parentUid);
    void fetchCollection();
    void waitForIt();
    void checkExitLoop();

    Akonadi::ETMCalendar *mCalendar;
    Akonadi::Collection mCollection;
    int mIncidencesToAdd;
    int mIncidencesToDelete;
    int mIncidencesToChange;
    QString mLastDeletedUid;
    QString mLastChangedUid;
};

#endif
