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

#include "todopurgertest.h"

#include "../etmcalendar.h"
#include "../todopurger.h"
#include <akonadi/itemcreatejob.h>
#include <akonadi/qtest_akonadi.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionfetchscope.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/itemdeletejob.h>
#include <akonadi/itemmodifyjob.h>
#include <KCheckableProxyModel>

#include <QTestEventLoop>

using namespace Akonadi;
using namespace KCalCore;

Q_DECLARE_METATYPE( QSet<QByteArray> )

void TodoPurgerTest::createTodo(const QString &uid, const QString &parentUid, bool completed, bool recurring )
{
    Item item;
    item.setMimeType(Todo::todoMimeType());
    Todo::Ptr todo = Todo::Ptr(new Todo());
    todo->setUid(uid);
    todo->setCompleted(completed);
    todo->setDtStart(KDateTime::currentDateTime(KDateTime::UTC));
    todo->setRelatedTo(parentUid);
    todo->setSummary(QLatin1String("summary"));
    if (recurring)
        todo->recurrence()->setDaily(1);

    item.setPayload<KCalCore::Incidence::Ptr>(todo);
    ItemCreateJob *job = new ItemCreateJob(item, m_collection, this);
    m_pendingCreations++;
    AKVERIFYEXEC(job);
}

void TodoPurgerTest::fetchCollection()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(),
                                                     CollectionFetchJob::Recursive,
                                                     this);
    // Get list of collections
    job->fetchScope().setContentMimeTypes(QStringList() << QLatin1String("application/x-vnd.akonadi.calendar.todo"));
    AKVERIFYEXEC(job);

    // Find our collection
    Collection::List collections = job->collections();
    QVERIFY(!collections.isEmpty());
    m_collection = collections.first();

    QVERIFY(m_collection.isValid());
}

void TodoPurgerTest::initTestCase()
{
    AkonadiTest::checkTestIsIsolated();

    qRegisterMetaType<QSet<QByteArray> >("QSet<QByteArray>");
    fetchCollection();

    m_pendingCreations = 0;
    m_pendingDeletions = 0;
    m_calendar = new ETMCalendar();
    m_calendar->registerObserver(this);
    m_todoPurger = new TodoPurger(this);

    connect(m_todoPurger, SIGNAL(todosPurged(bool,int,int)), SLOT(onTodosPurged(bool,int,int)));

    connect(m_calendar, SIGNAL(collectionsAdded(Akonadi::Collection::List)),
            &QTestEventLoop::instance(), SLOT(exitLoop()));

    // Wait for the collection
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());

    KCheckableProxyModel *checkable = m_calendar->checkableProxyModel();
    const QModelIndex firstIndex = checkable->index(0, 0);
    QVERIFY(firstIndex.isValid());
    checkable->setData(firstIndex, Qt::Checked, Qt::CheckStateRole);
}

void TodoPurgerTest::cleanupTestCase()
{
    delete m_calendar;
}

void TodoPurgerTest::testPurge()
{
    createTree();

    m_pendingDeletions = 8;
    m_pendingPurgeSignal = true;

    m_todoPurger->purgeCompletedTodos();

    // Wait for deletions and purged signal
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());

    QCOMPARE(m_numDeleted, 8);
    QCOMPARE(m_numIgnored, 8);
}

void TodoPurgerTest::calendarIncidenceAdded(const Incidence::Ptr &)
{
    --m_pendingCreations;
    if (m_pendingCreations == 0) {
        QTestEventLoop::instance().exitLoop();
    }
}

void TodoPurgerTest::calendarIncidenceDeleted(const Incidence::Ptr &)
{
    --m_pendingDeletions;
    if (m_pendingDeletions == 0 && !m_pendingPurgeSignal) {
        QTestEventLoop::instance().exitLoop();
    }
}

void TodoPurgerTest::onTodosPurged(bool success, int numDeleted, int numIgnored)
{
    QVERIFY(success);
    m_pendingPurgeSignal = false;
    m_numDeleted = numDeleted;
    m_numIgnored = numIgnored;

    if (m_pendingDeletions == 0) {
        QTestEventLoop::instance().enterLoop(10);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }
}

void TodoPurgerTest::createTree()
{
    createTodo(tr("a"), QString(), true); // top level to-do, completed
    createTodo(tr("b"), QString(), false); // top level to-do, un-complete

    // Completed tree
    createTodo(tr("c"), QString(), true);
    createTodo(tr("c1"), tr("c"), true);
    createTodo(tr("c1.1"), tr("c1"), true);
    createTodo(tr("c1.2"), tr("c1"), true);

    // Root completed but children not completed
    createTodo(tr("d"), QString(), true);
    createTodo(tr("d1"), tr("d"), false);
    createTodo(tr("d1.1"), tr("d1"), false);
    createTodo(tr("d1.2"), tr("d1"), false);

    // Root uncomplete with children complete
    createTodo(tr("e"), QString(), false);
    createTodo(tr("e1"), tr("e"), true);
    createTodo(tr("e1.1"), tr("e1"), true);
    createTodo(tr("e1.2"), tr("e1"), true);

    // Recurring uncomplete
    createTodo(tr("f"), QString(), false, true);

    // Recurring complete, this one is ignored too because recurrence didn't end
    createTodo(tr("g"), QString(), true, true);


    // Now wait for incidences do be created
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

QTEST_AKONADIMAIN(TodoPurgerTest, GUI)
