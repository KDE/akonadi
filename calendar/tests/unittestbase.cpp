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

#include <akonadi/item.h>
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
    qRegisterMetaType<Akonadi::ITIPHandler::Result>("Akonadi::ITIPHandler::Result");


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

/** static */
QByteArray UnitTestBase::readFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QByteArray();
    }

    return file.readAll();
}
