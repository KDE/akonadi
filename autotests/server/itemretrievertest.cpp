/*
    Copyright (c) 2013 Volker Krause <vkrause@kde.org>

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

#include <QObject>
#include <QtTest/QTest>
#include <QtCore/QBuffer>

#include "storage/itemretriever.h"

using namespace Akonadi::Server;

class ItemRetrieverTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testFullPayload()
    {
        ItemRetriever r1(0);
        r1.setRetrieveFullPayload(true);
        QCOMPARE(r1.retrieveParts().size(), 1);
        QCOMPARE(r1.retrieveParts().at(0), { "PLD:RFC822" });
        r1.setRetrieveParts({ "PLD:FOO" });
        QCOMPARE(r1.retrieveParts().size(), 2);
    }
};

QTEST_MAIN(ItemRetrieverTest)

#include "itemretrievertest.moc"
