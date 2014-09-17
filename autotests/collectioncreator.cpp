/*
    Copyright (c) 2006, 2009 Volker Krause <vkrause@kde.org>

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

#include "agentinstance.h"
#include "agentmanager.h"
#include "collectioncreatejob.h"
#include "collectionpathresolver.h"
#include "transactionjobs.h"

#include "qtest_akonadi.h"
#include "test_utils.h"

#include <QtCore/QDebug>

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

        QList<int> counts = QList<int>() << 1 << 10 << 100 << 1000;
        QList<bool> transactions = QList<bool>() << false << true;
        foreach (int count, counts) {
            foreach (bool transaction, transactions) {  //krazy:exclude=foreach
                QTest::newRow(QString::fromLatin1("%1-%2").arg(count).arg(transaction ? QLatin1String("trans") : QLatin1String("notrans")).toLatin1().constData())
                        << count << transaction;
            }
        }
    }

    void createCollections()
    {
        QFETCH(int, count);
        QFETCH(bool, useTransaction);

        const Collection parent(collectionIdFromPath(QLatin1String("res3")));
        QVERIFY(parent.isValid());

        static int index = 0;
        Job *lastJob = 0;
        QBENCHMARK
        {
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
            AkonadiTest::akWaitForSignal(lastJob, SIGNAL(result(KJob*)));
        }
    }
};

QTEST_AKONADIMAIN(CollectionCreator)

#include "collectioncreator.moc"
