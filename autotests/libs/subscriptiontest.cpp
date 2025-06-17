/*
    SPDX-FileCopyrightText: 2007 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collection.h"
#include "collectionfetchjob.h"
#include "collectionfetchscope.h"
#include "control.h"
#include "qtest_akonadi.h"
#include "subscriptionjob_p.h"

#include <QObject>

using namespace Akonadi;

class SubscriptionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        Control::start();
    }

    void testSubscribe()
    {
        Collection::List l;
        l << Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res2/foo2")));
        QVERIFY(l.first().isValid());
        auto sjob = new SubscriptionJob(this);
        sjob->unsubscribe(l);
        AKVERIFYEXEC(sjob);

        const Collection res2Col = Collection(AkonadiTest::collectionIdFromPath(QStringLiteral("res2")));
        QVERIFY(res2Col.isValid());
        auto ljob = new CollectionFetchJob(res2Col, CollectionFetchJob::FirstLevel, this);
        AKVERIFYEXEC(ljob);
        QCOMPARE(ljob->collections().count(), 1);

        ljob = new CollectionFetchJob(res2Col, CollectionFetchJob::FirstLevel, this);
        ljob->fetchScope().setListFilter(CollectionFetchScope::NoFilter);
        AKVERIFYEXEC(ljob);
        QCOMPARE(ljob->collections().count(), 2);

        sjob = new SubscriptionJob(this);
        sjob->subscribe(l);
        AKVERIFYEXEC(sjob);

        ljob = new CollectionFetchJob(res2Col, CollectionFetchJob::FirstLevel, this);
        AKVERIFYEXEC(ljob);
        QCOMPARE(ljob->collections().count(), 2);
    }

    void testEmptySubscribe()
    {
        Collection::List l;
        auto sjob = new SubscriptionJob(this);
        AKVERIFYEXEC(sjob);
    }

    void testInvalidSubscribe()
    {
        Collection::List l;
        l << Collection(1);
        auto sjob = new SubscriptionJob(this);
        sjob->subscribe(l);
        l << Collection(INT_MAX);
        sjob->unsubscribe(l);
        QVERIFY(!sjob->exec());
    }
};

QTEST_AKONADI_CORE_MAIN(SubscriptionTest)

#include "subscriptiontest.moc"
