/*
    SPDX-FileCopyrightText: 2006, 2009 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "agentinstance.h"
#include "agentmanager.h"
#include "collectioncreatejob.h"
#include "collectionpathresolver.h"
#include "transactionjobs.h"

#include "qtest_akonadi.h"

using namespace Akonadi;

class CollectionCreator : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
        AkonadiTest::setAllResourcesOffline();
    }

    void createCollections_data()
    {
        QTest::addColumn<int>("count");
        QTest::addColumn<bool>("useTransaction");

        const int counts[]{1, 10, 100, 1000};
        const bool transactions[]{false, true};
        for (int count : counts) {
            for (bool transaction : transactions) {
                QTest::newRow(
                    QString::fromLatin1("%1-%2").arg(count).arg(transaction ? QLatin1String("trans") : QLatin1String("notrans")).toLatin1().constData())
                    << count << transaction;
            }
        }
    }

    void createCollections()
    {
        QFETCH(int, count);
        QFETCH(bool, useTransaction);

        const Collection parent(AkonadiTest::collectionIdFromPath(QLatin1String("res3")));
        QVERIFY(parent.isValid());

        static int index = 0;
        Job *lastJob = 0;
        QBENCHMARK {
            if (useTransaction) {
                lastJob = new TransactionBeginJob(this);
            }
            for (int i = 0; i < count; ++i) {
                Collection col;
                col.setParentCollection(parent);
                col.setName(QLatin1String("col") + QString::number(++index));
                lastJob = new CollectionCreateJob(col, this);
            }
            if (useTransaction) {
                lastJob = new TransactionCommitJob(this);
            }
            AkonadiTest::akWaitForSignal(lastJob, SIGNAL(result(KJob *)), 15000);
        }
    }
};

QTEST_AKONADIMAIN(CollectionCreator)

#include "collectioncreator.moc"
