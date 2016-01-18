/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "newmailnotifierattributetest.h"
#include "newmailnotifierattribute.h"
#include <qtest.h>
using namespace Akonadi;
NewMailNotifierAttributeTest::NewMailNotifierAttributeTest(QObject *parent)
    : QObject(parent)
{

}

NewMailNotifierAttributeTest::~NewMailNotifierAttributeTest()
{

}

void NewMailNotifierAttributeTest::shouldHaveDefaultValue()
{
    NewMailNotifierAttribute attr;
    QVERIFY(!attr.ignoreNewMail());
}

void NewMailNotifierAttributeTest::shouldSetIgnoreNotification()
{
    NewMailNotifierAttribute attr;
    bool ignore = false;
    attr.setIgnoreNewMail(ignore);
    QCOMPARE(attr.ignoreNewMail(), ignore);
    ignore = true;
    attr.setIgnoreNewMail(ignore);
    QCOMPARE(attr.ignoreNewMail(), ignore);
}

void NewMailNotifierAttributeTest::shouldSerializedData()
{
    NewMailNotifierAttribute attr;
    attr.setIgnoreNewMail(true);
    QByteArray ba = attr.serialized();
    NewMailNotifierAttribute result;
    result.deserialize(ba);
    QVERIFY(attr == result);
}

void NewMailNotifierAttributeTest::shouldCloneAttribute()
{
    NewMailNotifierAttribute attr;
    attr.setIgnoreNewMail(true);
    NewMailNotifierAttribute *result = attr.clone();
    QVERIFY(attr == *result);
    delete result;
}

void NewMailNotifierAttributeTest::shouldHaveType()
{
    NewMailNotifierAttribute attr;
    QCOMPARE(attr.type(), QByteArray("newmailnotifierattribute"));
}

QTEST_MAIN(NewMailNotifierAttributeTest)
