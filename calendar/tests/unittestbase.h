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


#ifndef UNITTEST_BASE_H
#define UNITTEST_BASE_H

#include <kcalcore/calendar.h>
#include <akonadi/collection.h>
#include <akonadi/item.h>

#include <QObject>
#include <QString>

namespace Akonadi {
    class IncidenceChanger;
}

class UnitTestBase : public QObject {
    Q_OBJECT
public:
    UnitTestBase();
    void waitForIt(); // Waits 10 seconds for signals
    void stopWaiting();
    void createIncidence(const QString &uid);

    void verifyExists(const QString &uid, bool exists);
    Akonadi::Item::List calendarItems();

public Q_SLOTS:
    void onLoadFinished(bool success, const QString &errorMessage);

protected:

    void compareCalendars(const KCalCore::Calendar::Ptr &expectedCalendar);
    static QByteArray readFile(const QString &filename);
    static Akonadi::Item generateIncidence(const QString &uid, const QString &organizer = QString());

    Akonadi::Collection mCollection;
    Akonadi::IncidenceChanger *mChanger;
};

#endif
