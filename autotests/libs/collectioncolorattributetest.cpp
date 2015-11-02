/*
    Copyright (c) 2016 Sandro Knauß <knauss@kde.org>

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

#include "collectioncolorattribute.h"

#include <QtCore/QObject>

#include <QTest>

using namespace Akonadi;

class CollectionColorAttributeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testDeserialize_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QColor>("color");
        QTest::addColumn<QByteArray>("output");

        QTest::newRow("empty") << QByteArray("") << QColor() << QByteArray("");
        QTest::newRow("white") << QByteArray("white") << QColor("#ffffff") << QByteArray("#ffffffff");
        QTest::newRow("#123") << QByteArray("#123") << QColor("#112233") << QByteArray("#ff112233");
        QTest::newRow("#123456") << QByteArray("#123456") << QColor("#123456") << QByteArray("#ff123456");
        QTest::newRow("#1234567") << QByteArray("#1234567") << QColor() << QByteArray("");
        QTest::newRow("#12345678") << QByteArray("#12345678") << QColor("#12345678") << QByteArray("#12345678");
        QTest::newRow("#ff345678") << QByteArray("#ff123456") << QColor("#123456") << QByteArray("#ff123456");
    }

    void testDeserialize()
    {
        QFETCH(QByteArray, input);
        QFETCH(QColor, color);
        QFETCH(QByteArray, output);

        CollectionColorAttribute *attr = new CollectionColorAttribute();
        attr->deserialize(input);
        QCOMPARE(attr->color(), color);

        QCOMPARE(attr->serialized(), output);

        CollectionColorAttribute *copy = attr->clone();
        QCOMPARE(copy->serialized(), output);

        delete attr;
        delete copy;
    }
};

QTEST_MAIN(CollectionColorAttributeTest)

#include "collectioncolorattributetest.moc"

