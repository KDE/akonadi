/*
    SPDX-FileCopyrightText: 2009 Kevin Krammer <kevin.krammer@gmx.at>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "mimetypecheckertest.h"
#include "testattribute.h"

#include "collection.h"
#include "item.h"

#include <KRandom>

#include <QMimeDatabase>

#include <QTest>

#include <utility>

QTEST_MAIN(MimeTypeCheckerTest)

using namespace Akonadi;

MimeTypeCheckerTest::MimeTypeCheckerTest(QObject *parent)
    : QObject(parent)
{
    mCalendarSubTypes << QStringLiteral("application/x-vnd.akonadi.calendar.event") << QStringLiteral("application/x-vnd.akonadi.calendar.todo");
}

void MimeTypeCheckerTest::initTestCase()
{
    QVERIFY(QMimeDatabase().mimeTypeForName(QLatin1StringView("application/x-vnd.akonadi.calendar.event")).isValid());

    MimeTypeChecker emptyChecker;
    MimeTypeChecker calendarChecker;
    MimeTypeChecker subTypeChecker;
    MimeTypeChecker aliasChecker;

    // for testing reset through assignments
    const QLatin1StringView textPlain = QLatin1StringView("text/plain");
    mEmptyChecker.addWantedMimeType(textPlain);
    QVERIFY(!mEmptyChecker.wantedMimeTypes().isEmpty());
    QVERIFY(mEmptyChecker.hasWantedMimeTypes());

    const QLatin1StringView textCalendar = QLatin1StringView("text/calendar");
    calendarChecker.addWantedMimeType(textCalendar);
    QCOMPARE(calendarChecker.wantedMimeTypes().count(), 1);

    subTypeChecker.setWantedMimeTypes(mCalendarSubTypes);
    QCOMPARE(subTypeChecker.wantedMimeTypes().count(), 2);

    const QLatin1StringView textVCard = QLatin1StringView("text/directory");
    aliasChecker.addWantedMimeType(textVCard);
    QCOMPARE(aliasChecker.wantedMimeTypes().count(), 1);

    // test assignment works correctly
    mEmptyChecker = emptyChecker;
    mCalendarChecker = calendarChecker;
    mSubTypeChecker = subTypeChecker;
    mAliasChecker = aliasChecker;

    QVERIFY(mEmptyChecker.wantedMimeTypes().isEmpty());
    QVERIFY(!mEmptyChecker.hasWantedMimeTypes());

    QCOMPARE(mCalendarChecker.wantedMimeTypes().count(), 1);
    QCOMPARE(mCalendarChecker.wantedMimeTypes(), QStringList() << textCalendar);

    QCOMPARE(mSubTypeChecker.wantedMimeTypes().count(), 2);
    const auto calendarSubTypes = QSet<QString>(mCalendarSubTypes.begin(), mCalendarSubTypes.end());
    const auto wantedMimeTypes = mSubTypeChecker.wantedMimeTypes();
    const auto wantedSubTypes = QSet<QString>(wantedMimeTypes.begin(), wantedMimeTypes.end());
    QCOMPARE(wantedSubTypes, calendarSubTypes);

    QCOMPARE(mAliasChecker.wantedMimeTypes().count(), 1);
    QCOMPARE(mAliasChecker.wantedMimeTypes(), QStringList() << textVCard);
}

void MimeTypeCheckerTest::testCollectionCheck()
{
    Collection invalidCollection;
    Collection emptyCollection(1);
    Collection calendarCollection(2);
    Collection eventCollection(3);
    Collection journalCollection(4);
    Collection vcardCollection(5);
    Collection aliasCollection(6);

    const QLatin1StringView textCalendar = QLatin1StringView("text/calendar");
    calendarCollection.setContentMimeTypes(QStringList() << textCalendar);
    const QLatin1StringView akonadiEvent = QLatin1StringView("application/x-vnd.akonadi.calendar.event");
    eventCollection.setContentMimeTypes(QStringList() << akonadiEvent);
    journalCollection.setContentMimeTypes(QStringList() << QStringLiteral("application/x-vnd.akonadi.calendar.journal"));
    const QLatin1StringView textDirectory = QLatin1StringView("text/directory");
    vcardCollection.setContentMimeTypes(QStringList() << textDirectory);
    aliasCollection.setContentMimeTypes(QStringList() << QStringLiteral("text/x-vcard"));

    Collection::List voidCollections;
    voidCollections << invalidCollection << emptyCollection;

    Collection::List subTypeCollections;
    subTypeCollections << eventCollection << journalCollection;

    Collection::List calendarCollections = subTypeCollections;
    calendarCollections << calendarCollection;

    Collection::List contactCollections;
    contactCollections << vcardCollection << aliasCollection;

    //// empty checker fails for all
    Collection::List collections = voidCollections + calendarCollections + contactCollections;
    for (const Collection &collection : std::as_const(collections)) {
        QVERIFY(!mEmptyChecker.isWantedCollection(collection));
        QVERIFY(!MimeTypeChecker::isWantedCollection(collection, QString()));
    }

    //// calendar checker fails for void and contact collections
    collections = voidCollections + contactCollections;
    for (const Collection &collection : std::as_const(collections)) {
        QVERIFY(!mCalendarChecker.isWantedCollection(collection));
        QVERIFY(!MimeTypeChecker::isWantedCollection(collection, textCalendar));
    }

    // but accepts all calendar collections
    collections = calendarCollections;
    for (const Collection &collection : std::as_const(collections)) {
        QVERIFY(mCalendarChecker.isWantedCollection(collection));
        QVERIFY(MimeTypeChecker::isWantedCollection(collection, textCalendar));
    }

    //// sub type checker fails for all but the event collection
    collections = voidCollections + calendarCollections + contactCollections;
    collections.removeAll(eventCollection);
    for (const Collection &collection : std::as_const(collections)) {
        QVERIFY(!mSubTypeChecker.isWantedCollection(collection));
        QVERIFY(!MimeTypeChecker::isWantedCollection(collection, akonadiEvent));
    }

    // but accepts the event collection
    collections = Collection::List() << eventCollection;
    for (const Collection &collection : std::as_const(collections)) {
        QVERIFY(mSubTypeChecker.isWantedCollection(collection));
        QVERIFY(MimeTypeChecker::isWantedCollection(collection, akonadiEvent));
    }

    //// alias checker fails for void and calendar collections
    collections = voidCollections + calendarCollections;
    for (const Collection &collection : std::as_const(collections)) {
        QVERIFY(!mAliasChecker.isWantedCollection(collection));
        QVERIFY(!MimeTypeChecker::isWantedCollection(collection, textDirectory));
    }

    // but accepts all contact collections
    collections = contactCollections;
    for (const Collection &collection : std::as_const(collections)) {
        QVERIFY(mAliasChecker.isWantedCollection(collection));
        QVERIFY(MimeTypeChecker::isWantedCollection(collection, textDirectory));
    }
}

void MimeTypeCheckerTest::testItemCheck()
{
    Item invalidItem;
    Item emptyItem(1);
    Item calendarItem(2);
    Item eventItem(3);
    Item journalItem(4);
    Item vcardItem(5);
    Item aliasItem(6);

    const QLatin1StringView textCalendar = QLatin1StringView("text/calendar");
    calendarItem.setMimeType(textCalendar);
    const QLatin1StringView akonadiEvent = QLatin1StringView("application/x-vnd.akonadi.calendar.event");
    eventItem.setMimeType(akonadiEvent);
    journalItem.setMimeType(QStringLiteral("application/x-vnd.akonadi.calendar.journal"));
    const QLatin1StringView textDirectory = QLatin1StringView("text/directory");
    vcardItem.setMimeType(textDirectory);
    aliasItem.setMimeType(QStringLiteral("text/x-vcard"));

    Item::List voidItems;
    voidItems << invalidItem << emptyItem;

    Item::List subTypeItems;
    subTypeItems << eventItem << journalItem;

    Item::List calendarItems = subTypeItems;
    calendarItems << calendarItem;

    Item::List contactItems;
    contactItems << vcardItem << aliasItem;

    //// empty checker fails for all
    Item::List items = voidItems + calendarItems + contactItems;
    for (const Item &item : std::as_const(items)) {
        QVERIFY(!mEmptyChecker.isWantedItem(item));
        QVERIFY(!MimeTypeChecker::isWantedItem(item, QString()));
    }

    //// calendar checker fails for void and contact items
    items = voidItems + contactItems;
    for (const Item &item : std::as_const(items)) {
        QVERIFY(!mCalendarChecker.isWantedItem(item));
        QVERIFY(!MimeTypeChecker::isWantedItem(item, textCalendar));
    }

    // but accepts all calendar items
    items = calendarItems;
    for (const Item &item : std::as_const(items)) {
        QVERIFY(mCalendarChecker.isWantedItem(item));
        QVERIFY(MimeTypeChecker::isWantedItem(item, textCalendar));
    }

    //// sub type checker fails for all but the event item
    items = voidItems + calendarItems + contactItems;
    items.removeAll(eventItem);
    for (const Item &item : std::as_const(items)) {
        QVERIFY(!mSubTypeChecker.isWantedItem(item));
        QVERIFY(!MimeTypeChecker::isWantedItem(item, akonadiEvent));
    }

    // but accepts the event item
    items = Item::List() << eventItem;
    for (const Item &item : std::as_const(items)) {
        QVERIFY(mSubTypeChecker.isWantedItem(item));
        QVERIFY(MimeTypeChecker::isWantedItem(item, akonadiEvent));
    }

    //// alias checker fails for void and calendar items
    items = voidItems + calendarItems;
    for (const Item &item : std::as_const(items)) {
        QVERIFY(!mAliasChecker.isWantedItem(item));
        QVERIFY(!MimeTypeChecker::isWantedItem(item, textDirectory));
    }

    // but accepts all contact items
    items = contactItems;
    for (const Item &item : std::as_const(items)) {
        QVERIFY(mAliasChecker.isWantedItem(item));
        QVERIFY(MimeTypeChecker::isWantedItem(item, textDirectory));
    }
}

void MimeTypeCheckerTest::testStringMatchEquivalent()
{
    // check that a random and thus not installed MIME type
    // can still be checked just like with direct string comparison

    const QLatin1StringView installedMimeType("text/plain");
    const QString randomMimeType = QLatin1StringView("application/x-vnd.test.") + KRandom::randomString(10);

    MimeTypeChecker installedTypeChecker;
    installedTypeChecker.addWantedMimeType(installedMimeType);

    MimeTypeChecker randomTypeChecker;
    randomTypeChecker.addWantedMimeType(randomMimeType);

    Item item1(1);
    item1.setMimeType(installedMimeType);
    Item item2(2);
    item2.setMimeType(randomMimeType);

    Collection collection1(1);
    collection1.setContentMimeTypes(QStringList() << installedMimeType);
    Collection collection2(2);
    collection2.setContentMimeTypes(QStringList() << randomMimeType);
    Collection collection3(3);
    collection3.setContentMimeTypes(QStringList() << installedMimeType << randomMimeType);

    QVERIFY(installedTypeChecker.isWantedItem(item1));
    QVERIFY(!randomTypeChecker.isWantedItem(item1));
    QVERIFY(MimeTypeChecker::isWantedItem(item1, installedMimeType));
    QVERIFY(!MimeTypeChecker::isWantedItem(item1, randomMimeType));

    QVERIFY(!installedTypeChecker.isWantedItem(item2));
    QVERIFY(randomTypeChecker.isWantedItem(item2));
    QVERIFY(!MimeTypeChecker::isWantedItem(item2, installedMimeType));
    QVERIFY(MimeTypeChecker::isWantedItem(item2, randomMimeType));

    QVERIFY(installedTypeChecker.isWantedCollection(collection1));
    QVERIFY(!randomTypeChecker.isWantedCollection(collection1));
    QVERIFY(MimeTypeChecker::isWantedCollection(collection1, installedMimeType));
    QVERIFY(!MimeTypeChecker::isWantedCollection(collection1, randomMimeType));

    QVERIFY(!installedTypeChecker.isWantedCollection(collection2));
    QVERIFY(randomTypeChecker.isWantedCollection(collection2));
    QVERIFY(!MimeTypeChecker::isWantedCollection(collection2, installedMimeType));
    QVERIFY(MimeTypeChecker::isWantedCollection(collection2, randomMimeType));

    QVERIFY(installedTypeChecker.isWantedCollection(collection3));
    QVERIFY(randomTypeChecker.isWantedCollection(collection3));
    QVERIFY(MimeTypeChecker::isWantedCollection(collection3, installedMimeType));
    QVERIFY(MimeTypeChecker::isWantedCollection(collection3, randomMimeType));
}

#include "moc_mimetypecheckertest.cpp"
