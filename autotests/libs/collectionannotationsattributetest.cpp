/*
    SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <Akonadi/CollectionAnnotationsAttribute>

#include <QTest>

using Annotation = QMap<QByteArray, QByteArray>;
Q_DECLARE_METATYPE(Annotation)

using namespace Akonadi;

class CollectionAnnotationsAttributeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testSerializeDeserialize_data()
    {
        QTest::addColumn<Annotation>("annotation");

        Annotation a;
        QTest::newRow("empty") << a;

        a.insert("/vendor/cmu/cyrus-imapd/lastpop", "");
        QTest::newRow("empty value, single key") << a;

        a.insert("/vendor/cmu/cyrus-imapd/condstore", "false");
        QTest::newRow("empty value, two keys") << a;

        a.insert("/vendor/cmu/cyrus-imapd/sharedseen", "false");
        QTest::newRow("empty value, three keys") << a;

        a.clear();
        a.insert("vendor/cmu/cyrus-imapd/lastpop", " ");
        QTest::newRow("space value, single key") << a;

        a.insert("/vendor/cmu/cyrus-imapd/condstore", "false");
        QTest::newRow("space value, two keys") << a;

        a.insert("/vendor/cmu/cyrus-imapd/sharedseen", "false");
        QTest::newRow("space value, three keys") << a;
    }

    void testSerializeDeserialize()
    {
        QFETCH(Annotation, annotation);
        auto attr1 = new CollectionAnnotationsAttribute();
        attr1->setAnnotations(annotation);
        QCOMPARE(attr1->annotations(), annotation);

        auto attr2 = new CollectionAnnotationsAttribute();
        attr2->deserialize(attr1->serialized());
        QCOMPARE(attr2->annotations(), annotation);

        auto attr3 = new CollectionAnnotationsAttribute();
        attr3->setAnnotations(attr2->annotations());
        QCOMPARE(attr3->serialized(), attr1->serialized());

        delete attr1;
        delete attr2;
        delete attr3;
    }

    void shouldHaveDefaultValue()
    {
        CollectionAnnotationsAttribute attr;
        QVERIFY(attr.annotations().isEmpty());
    }

    void shouldAssignAttribute()
    {
        QMap<QByteArray, QByteArray> annotations;
        annotations.insert("foo", "foo");
        annotations.insert("bla", "bla");
        annotations.insert("foo2", "bli");
        CollectionAnnotationsAttribute attr(annotations);
        QCOMPARE(attr.annotations(), annotations);
    }

    void shouldDeserializedAttribute()
    {
        QMap<QByteArray, QByteArray> annotations;
        annotations.insert("foo", "foo");
        annotations.insert("bla", "bla");
        annotations.insert("foo2", "bli");
        CollectionAnnotationsAttribute attr(annotations);
        const QByteArray ba = attr.serialized();
        CollectionAnnotationsAttribute result;
        result.deserialize(ba);
        QVERIFY(attr == result);
    }

    void shouldCloneAttribute()
    {
        QMap<QByteArray, QByteArray> annotations;
        annotations.insert("foo", "foo");
        annotations.insert("bla", "bla");
        annotations.insert("foo2", "bli");
        CollectionAnnotationsAttribute attr(annotations);
        CollectionAnnotationsAttribute *result = attr.clone();
        QVERIFY(attr == *result);
        delete result;
    }

    void shouldHaveType()
    {
        CollectionAnnotationsAttribute attr;
        QCOMPARE(attr.type(), QByteArray("collectionannotations"));
    }
};

QTEST_GUILESS_MAIN(CollectionAnnotationsAttributeTest)

#include "collectionannotationsattributetest.moc"
