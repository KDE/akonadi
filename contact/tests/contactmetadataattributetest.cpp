/*
    This file is part of Akonadi Contact.

    Copyright (c) 2009 Tobias Koenig <tokoe@kde.org>

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

#include "contactmetadataattributetest.h"

#include "contactmetadataattribute_p.h"

#include <qtest_kde.h>

QTEST_KDEMAIN(ContactMetaDataAttributeTest, NoGUI)

static QVariantMap testData()
{
    QVariantMap data;
    data.insert(QStringLiteral("key1"), QStringLiteral("value1"));
    data.insert(QStringLiteral("key2"), QStringLiteral("value2"));

    return data;
}

void ContactMetaDataAttributeTest::type()
{
    Akonadi::ContactMetaDataAttribute attribute;

    QVERIFY(attribute.type() == "contactmetadata");
}

void ContactMetaDataAttributeTest::clone()
{
    const QVariantMap content1 = testData();

    Akonadi::ContactMetaDataAttribute attribute1;
    attribute1.setMetaData(content1);

    Akonadi::ContactMetaDataAttribute *attribute2 = static_cast<Akonadi::ContactMetaDataAttribute *>(attribute1.clone());
    const QVariantMap content2 = attribute2->metaData();

    QVERIFY(content1 == content2);
}

void ContactMetaDataAttributeTest::serialization()
{
    const QVariantMap content1 = testData();

    Akonadi::ContactMetaDataAttribute attribute1;
    attribute1.setMetaData(content1);

    const QByteArray data = attribute1.serialized();

    Akonadi::ContactMetaDataAttribute attribute2;
    attribute2.deserialize(data);

    const QVariantMap content2 = attribute2.metaData();

    QVERIFY(content1 == content2);
}
