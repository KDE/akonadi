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


#include "unittestbase.h"
#include "helper.h"
#include "../fetchjobcalendar.h"

#include <kcalcore/event.h>
#include <akonadi/item.h>
#include <akonadi/itemcreatejob.h>
#include <akonadi/calendar/incidencechanger.h>
#include <akonadi/calendar/itiphandler.h>

#include <QString>
#include <QFile>
#include <QByteArray>
#include <QTestEventLoop>
#include <qtest.h>

using namespace Akonadi;

UnitTestBase::UnitTestBase()
{
    qRegisterMetaType<Akonadi::Item>("Akonadi::Item");
    qRegisterMetaType<QList<Akonadi::IncidenceChanger::ChangeType> >("QList<Akonadi::IncidenceChanger::ChangeType>");
    qRegisterMetaType<QVector<Akonadi::Item::Id> >("QVector<Akonadi::Item::Id>");

    mChanger = new IncidenceChanger(this);
    mChanger->setShowDialogsOnError(false);
    mChanger->setHistoryEnabled(true);

    mCollection = Helper::fetchCollection();
    Q_ASSERT(mCollection.isValid());
    mChanger->setDefaultCollection(mCollection);
}

void UnitTestBase::waitForIt()
{
    QTestEventLoop::instance().enterLoop(10);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

void UnitTestBase::stopWaiting()
{
    QTestEventLoop::instance().exitLoop();
}

void UnitTestBase::createIncidence(const QString &uid)
{
    Item item;
    item.setMimeType(KCalCore::Event::eventMimeType());
    KCalCore::Incidence::Ptr incidence = KCalCore::Incidence::Ptr(new KCalCore::Event());
    incidence->setUid(uid);
    incidence->setDtStart(KDateTime::currentUtcDateTime());
    incidence->setSummary(QLatin1String("summary"));
    item.setPayload<KCalCore::Incidence::Ptr>(incidence);
    QVERIFY(mCollection.isValid());
    ItemCreateJob *job = new ItemCreateJob(item, mCollection, this);
    QVERIFY(job->exec());
}

void UnitTestBase::verifyExists(const QString &uid, bool exists)
{
    FetchJobCalendar *calendar = new FetchJobCalendar();
    connect(calendar, SIGNAL(loadFinished(bool,QString)), SLOT(onLoadFinished(bool,QString)));
    waitForIt();
    calendar->deleteLater();

    QCOMPARE(calendar->incidence(uid) != 0, exists);
}

Akonadi::Item::List UnitTestBase::calendarItems()
{
    FetchJobCalendar *calendar = new FetchJobCalendar();
    connect(calendar, SIGNAL(loadFinished(bool,QString)), SLOT(onLoadFinished(bool,QString)));
    waitForIt();
    calendar->deleteLater();
    return calendar->items();
}

void UnitTestBase::onLoadFinished(bool success, const QString &)
{
    QVERIFY(success);
    stopWaiting();
}

/** static */
QByteArray UnitTestBase::readFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QByteArray();
    }

    return file.readAll();
}
