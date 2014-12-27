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

#include "pop3resourceattributetest.h"
#include "pop3resourceattribute.h"
#include <qtest.h>
Pop3ResourceAttributeTest::Pop3ResourceAttributeTest(QObject *parent)
    : QObject(parent)
{

}

Pop3ResourceAttributeTest::~Pop3ResourceAttributeTest()
{

}

void Pop3ResourceAttributeTest::shouldHaveDefaultValue()
{
    Akonadi::Pop3ResourceAttribute attr;
    QVERIFY(attr.pop3AccountName().isEmpty());
}

void Pop3ResourceAttributeTest::shouldAssignValue()
{
    Akonadi::Pop3ResourceAttribute attr;
    QString accountName;
    attr.setPop3AccountName(accountName);
    QCOMPARE(attr.pop3AccountName(), accountName);
    accountName = QStringLiteral("foo");
    attr.setPop3AccountName(accountName);
    QCOMPARE(attr.pop3AccountName(), accountName);
    accountName.clear();
    attr.setPop3AccountName(accountName);
    QCOMPARE(attr.pop3AccountName(), accountName);
}

void Pop3ResourceAttributeTest::shouldDeserializeValue()
{
    Akonadi::Pop3ResourceAttribute attr;
    QString accountName = QStringLiteral("foo");
    attr.setPop3AccountName(accountName);
    const QByteArray ba = attr.serialized();
    Akonadi::Pop3ResourceAttribute result;
    result.deserialize(ba);
    QVERIFY(attr == result);
}

void Pop3ResourceAttributeTest::shouldCloneAttribute()
{
    Akonadi::Pop3ResourceAttribute attr;
    QString accountName = QStringLiteral("foo");
    attr.setPop3AccountName(accountName);
    Akonadi::Pop3ResourceAttribute *result = attr.clone();
    QVERIFY(attr == *result);
    delete result;
}

QTEST_MAIN(Pop3ResourceAttributeTest)
