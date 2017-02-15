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
#include <QTest>
#include <QTimer>
#include <QMutex>

#include "storage/itemretriever.h"
#include "storage/itemretrievaljob.h"
#include "storage/itemretrievalmanager.h"
#include "storage/itemretrievalrequest.h"

#include "fakeakonadiserver.h"
#include "dbinitializer.h"

#include <aktest.h>

using namespace Akonadi::Server;

struct JobResult
{
    qint64 pimItemId;
    QByteArray partname;
    QByteArray partdata;
    QString error;
};

class FakeItemRetrievalJob : public AbstractItemRetrievalJob
{
    Q_OBJECT
public:
    FakeItemRetrievalJob(ItemRetrievalRequest *req, DbInitializer &dbInitializer,
                         const QVector<JobResult> &results, QObject *parent)
        : AbstractItemRetrievalJob(req, parent)
        , mDbInitializer(dbInitializer)
        , mResults(results)
    {
    }

    void start() Q_DECL_OVERRIDE
    {
        Q_FOREACH (const JobResult &res, mResults) {
            if (res.error.isEmpty()) {
                // This is analogous to what STORE/MERGE does
                const PimItem item = PimItem::retrieveById(res.pimItemId);
                const auto parts = item.parts();
                // Try to find the part by name
                auto it = std::find_if(parts.begin(), parts.end(),
                                       [res](const Part &part) {
                                           return part.partType().name().toLatin1() == res.partname;
                                       });
                if (it == parts.end()) {
                    // Does not exist, create it
                    mDbInitializer.createPart(res.pimItemId, "PLD:" + res.partname, res.partdata);
                } else {
                    // Exist, update it
                    Part part(*it);
                    part.setData(res.partdata);
                    part.setDatasize(res.partdata.size());
                    part.update();
                }
            } else {
                mError = res.error;
                break;
            }
        }

        QTimer::singleShot(0, this, [this]() {
            Q_EMIT requestCompleted(m_request, mError);
        });
    }

    void kill() Q_DECL_OVERRIDE
    {
        // TODO?
    }

private:
    DbInitializer &mDbInitializer;
    QVector<JobResult> mResults;
    QString mError;
};

class FakeItemRetrievalJobFactory : public AbstractItemRetrievalJobFactory
{
public:
    FakeItemRetrievalJobFactory(DbInitializer &initializer)
        : mJobsCount(0)
        , mDbInitializer(initializer)
    {
    }

    void addJobResult(qint64 itemId, const QByteArray &partname, const QByteArray &partdata)
    {
        mJobResults.insert(itemId, JobResult{ itemId, partname, partdata, QString() });
    }

    void addJobResult(qint64 itemId, const QString &error)
    {
        mJobResults.insert(itemId, JobResult{ itemId, QByteArray(), QByteArray(), error });
    }

    AbstractItemRetrievalJob *retrievalJob(ItemRetrievalRequest *request, QObject *parent) Q_DECL_OVERRIDE
    {
        QVector<JobResult> results;
        Q_FOREACH (auto id, request->ids) {
            auto it = mJobResults.constFind(id);
            while (it != mJobResults.constEnd() && it.key() == id) {
                if (request->parts.contains(it->partname)) {
                    results << *it;
                }
                ++it;
            }
        }

        ++mJobsCount;
        return new FakeItemRetrievalJob(request, mDbInitializer, results, parent);
    }

    int jobsCount() const
    {
        return mJobsCount;
    }

private:
    int mJobsCount;
    DbInitializer &mDbInitializer;
    QMultiHash<qint64, JobResult> mJobResults;
};

using RequestedParts = QVector<QByteArray /* FQ name */>;

class ClientThread : public QThread
{
public:
    ClientThread(Entity::Id itemId, const RequestedParts &requestedParts)
        : m_itemId(itemId), m_requestedParts(requestedParts)
    {}

    void run() Q_DECL_OVERRIDE
    {
        // ItemRetriever should...
        ItemRetriever retriever;
        retriever.setItem(m_itemId);
        retriever.setRetrieveParts(m_requestedParts);
        QSignalSpy spy(&retriever, &ItemRetriever::itemsRetrieved);

        const bool success = retriever.exec();

        QMutexLocker lock(&m_mutex);
        m_results.success = success;
        m_results.signalsCount = spy.count();
        if (m_results.signalsCount > 0) {
            m_results.emittedItems = spy.at(0).at(0).value<QList<qint64>>();
        }
    }

    struct Results
    {
        bool success;
        int signalsCount;
        QList<qint64> emittedItems;
    };
    Results results() const {
        QMutexLocker lock(&m_mutex);
        return m_results;
    }

private:
    const Entity::Id m_itemId;
    const RequestedParts m_requestedParts;

    mutable QMutex m_mutex; // protects results below
    Results m_results;
};

class ItemRetrieverTest : public QObject
{
    Q_OBJECT


    using ExistingParts = QVector<QPair<QByteArray /* name */, QByteArray /* data */>>;
    using AvailableParts = QVector<QPair<QByteArray /* name */, QByteArray /* data */>>;

public:
    ItemRetrieverTest()
        : QObject()
    {
        try {
            FakeAkonadiServer::instance()->setPopulateDb(false);
            FakeAkonadiServer::instance()->disableItemRetrievalManager();
            FakeAkonadiServer::instance()->init();
        } catch (const FakeAkonadiServerException &e) {
            qWarning() << "Server exception: " << e.what();
            qFatal("Fake Akonadi Server failed to start up, aborting test");
        }
    }

    ~ItemRetrieverTest()
    {
        FakeAkonadiServer::instance()->quit();
    }

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

    void testRetrieval_data()
    {
        QTest::addColumn<ExistingParts>("existingParts");
        QTest::addColumn<AvailableParts>("availableParts");
        QTest::addColumn<RequestedParts>("requestedParts");
        QTest::addColumn<int>("expectedRetrievalJobs");
        QTest::addColumn<int>("expectedSignals");
        QTest::addColumn<int>("expectedParts");

        QTest::newRow("should retrieve missing payload part")
            << ExistingParts()
            << AvailableParts{ { "RFC822", "somedata" } }
            << RequestedParts{ "PLD:RFC822" }
            << 1 << 1 << 1;

        QTest::newRow("should retrieve multiple missing payload parts")
            << ExistingParts()
            << AvailableParts{ { "RFC822", "somedata" }, { "HEAD", "head" } }
            << RequestedParts{ "PLD:HEAD", "PLD:RFC822" }
            << 1 << 1 << 2;

        QTest::newRow("should not retrieve existing payload part")
            << ExistingParts{ { "PLD:RFC822", "somedata" } }
            << AvailableParts()
            << RequestedParts{ "PLD:RFC822" }
            << 0 << 1 << 1;

        QTest::newRow("should not retrieve multiple existing payload parts")
            << ExistingParts{ { "PLD:RFC822", "somedata" }, { "PLD:HEAD", "head" } }
            << AvailableParts()
            << RequestedParts{ "PLD:RFC822", "PLD:HEAD" }
            << 0 << 1 << 2;

        QTest::newRow("should retrieve missing but not existing payload part")
            << ExistingParts{ { "PLD:HEAD", "head" } }
            << AvailableParts{ { "RFC822", "somedata" } }
            << RequestedParts{ "PLD:HEAD", "PLD:RFC822" }
            << 1 << 1 << 2;

        QTest::newRow("should retrieve expired payload part")
            << ExistingParts{ { "PLD:RFC822", QByteArray() } }
            << AvailableParts{ { "RFC822", "somedata" } }
            << RequestedParts{ "PLD:RFc822" }
            << 1 << 1 << 1;

        QTest::newRow("should not retrieve one out of multiple existing payload parts")
            << ExistingParts{ { "PLD:RFC822", "somedata" }, { "PLD:HEAD", "head" }, { "PLD:ENVELOPE", "envelope" } }
            << AvailableParts()
            << RequestedParts{ "PLD:RFC822", "PLD:HEAD" }
            << 0 << 1 << 3;

        QTest::newRow("should retrieve missing payload part and ignore attributes")
            << ExistingParts{ { "ATR:MYATTR", "myattrdata" } }
            << AvailableParts{ { "RFC822", "somedata" } }
            << RequestedParts{ "PLD:RFC822" }
            << 1 << 1 << 2;
    }

    void testRetrieval()
    {
        QFETCH(ExistingParts, existingParts);
        QFETCH(AvailableParts, availableParts);
        QFETCH(RequestedParts, requestedParts);
        QFETCH(int, expectedRetrievalJobs);
        QFETCH(int, expectedSignals);
        QFETCH(int, expectedParts);


        // Setup
        for (int step = 0; step < 2; ++step) {
            DbInitializer dbInitializer;
            FakeItemRetrievalJobFactory factory(dbInitializer);
            ItemRetrievalManager mgr(&factory);
            QTest::qWait(100);

            // Given a PimItem with existing parts
            Resource res = dbInitializer.createResource("testresource");
            Collection col = dbInitializer.createCollection("col1");

            // step 0: do it in the main thread, for easier debugging
            PimItem item = dbInitializer.createItem("1", col);
            Q_FOREACH (const auto &existingPart, existingParts) {
                dbInitializer.createPart(item.id(), existingPart.first, existingPart.second);
            }

            Q_FOREACH (const auto &availablePart, availableParts) {
                factory.addJobResult(item.id(), availablePart.first, availablePart.second);
            }

            if (step == 0) {
                ClientThread thread(item.id(), requestedParts);
                thread.run();

                const ClientThread::Results results = thread.results();
                // ItemRetriever should ... succeed
                QVERIFY(results.success);
                // Emit exactly one signal ...
                QCOMPARE(results.signalsCount, expectedSignals);
                // ... with that one item
                if (expectedSignals > 0) {
                    QCOMPARE(results.emittedItems, QList<qint64>{ item.id() });
                }

                // Check that the factory had exactly one retrieval job
                QCOMPARE(factory.jobsCount(), expectedRetrievalJobs);

            } else {
                QVector<ClientThread *> threads;
                for (int i = 0; i < 20; ++i) {
                    threads.append(new ClientThread(item.id(), requestedParts));
                }
                for (int i = 0; i < threads.size(); ++i) {
                    threads.at(i)->start();
                }
                for (int i = 0; i < threads.size(); ++i) {
                    threads.at(i)->wait();
                }
                for (int i = 0; i < threads.size(); ++i) {
                    const ClientThread::Results results = threads.at(i)->results();
                    QVERIFY(results.success);
                    QCOMPARE(results.signalsCount, expectedSignals);
                    if (expectedSignals > 0) {
                        QCOMPARE(results.emittedItems, QList<qint64>{ item.id() });
                    }
                }
                qDeleteAll(threads);
            }

            // Check that the parts now exist in the DB
            const auto parts = item.parts();
            QCOMPARE(parts.count(), expectedParts);
            Q_FOREACH (const Part &dbPart, item.parts()) {
                const QString fqname = dbPart.partType().ns() + QLatin1Char(':') + dbPart.partType().name();
                if (!requestedParts.contains(fqname.toLatin1())) {
                    continue;
                }

                auto it = std::find_if(availableParts.constBegin(), availableParts.constEnd(),
                        [dbPart](const QPair<QByteArray, QByteArray> &p) {
                        return dbPart.partType().name().toLatin1() == p.first;
                        });
                if (it == availableParts.constEnd()) {
                    it = std::find_if(existingParts.constBegin(), existingParts.constEnd(),
                            [fqname](const QPair<QByteArray, QByteArray> &p) {
                            return fqname.toLatin1() == p.first;
                            });
                    QVERIFY(it != existingParts.constEnd());
                }

                QCOMPARE(dbPart.data(), it->second);
                QCOMPARE(dbPart.datasize(), it->second.size());
            }
        }
    }
};

AKTEST_FAKESERVER_MAIN(ItemRetrieverTest)

#include "itemretrievertest.moc"
