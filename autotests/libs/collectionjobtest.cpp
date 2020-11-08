/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectionjobtest.h"

#include <sys/types.h>

#include <qtest_akonadi.h>
#include "testattribute.h"

#include "agentmanager.h"
#include "agentinstance.h"
#include "attributefactory.h"
#include "cachepolicy.h"
#include "collection.h"
#include "collectioncreatejob.h"
#include "collectiondeletejob.h"
#include "collectionfetchjob.h"
#include "collectionmodifyjob.h"
#include "collectionstatistics.h"
#include "collectionstatisticsjob.h"
#include "collectionutils.h"
#include "control.h"
#include "item.h"
#include "resourceselectjob_p.h"
#include "collectionfetchscope.h"

using namespace Akonadi;

QTEST_AKONADIMAIN(CollectionJobTest)

void CollectionJobTest::initTestCase()
{
    qRegisterMetaType<Akonadi::Collection::List>();
    AttributeFactory::registerAttribute<TestAttribute>();
    AkonadiTest::checkTestIsIsolated();
    Control::start();
    AkonadiTest::setAllResourcesOffline();
}

static Collection findCol(const Collection::List &list, const QString &name)
{
    foreach (const Collection &col, list)
        if (col.name() == name) {
            return col;
        }
    return Collection();
}

// list compare which ignores the order
template <class T> static void compareLists(const QList<T> &l1, const QList<T> &l2)
{
    QCOMPARE(l1.count(), l2.count());
    foreach (const T &entry, l1) {
        QVERIFY(l2.contains(entry));
    }
}

template <class T> static void compareLists(const QVector<T> &l1, const QVector<T> &l2)
{
    QCOMPARE(l1.count(), l2.count());
    foreach (const T &entry, l1) {
        QVERIFY(l2.contains(entry));
    }
}

template <typename T> static T *extractAttribute(QList<Attribute *> attrs)
{
    T dummy;
    foreach (Attribute *attr, attrs) {
        if (attr->type() == dummy.type()) {
            return dynamic_cast<T *>(attr);
        }
    }
    return 0;
}

static Collection::Id res1ColId = 6; // -1;
static Collection::Id res2ColId = 7; //-1;
static Collection::Id res3ColId = -1;
static Collection::Id searchColId = -1;

void CollectionJobTest::testTopLevelList()
{
    // non-recursive top-level list
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel);
    AKVERIFYEXEC(job);
    Collection::List list = job->collections();

    // check if everything is there and has the correct types and attributes
    QCOMPARE(list.count(), 4);
    Collection col;

    col = findCol(list, QStringLiteral("res1"));
    QVERIFY(col.isValid());
    res1ColId = col.id(); // for the next test
    QVERIFY(res1ColId > 0);
    QVERIFY(CollectionUtils::isResource(col));
    QCOMPARE(col.parentCollection(), Collection::root());
    QCOMPARE(col.resource(), QStringLiteral("akonadi_knut_resource_0"));

    QVERIFY(findCol(list, QStringLiteral("res2")).isValid());
    res2ColId = findCol(list, QStringLiteral("res2")).id();
    QVERIFY(res2ColId > 0);
    QVERIFY(findCol(list, QStringLiteral("res3")).isValid());
    res3ColId = findCol(list, QStringLiteral("res3")).id();
    QVERIFY(res3ColId > 0);

    col = findCol(list, QStringLiteral("Search"));
    searchColId = col.id();
    QVERIFY(col.isValid());
    QVERIFY(CollectionUtils::isVirtualParent(col));
    QCOMPARE(col.resource(), QStringLiteral("akonadi_search_resource"));
}

void CollectionJobTest::testFolderList()
{
    // recursive list of physical folders
    CollectionFetchJob *job = new CollectionFetchJob(Collection(res1ColId), CollectionFetchJob::Recursive);
    QSignalSpy spy(job, &CollectionFetchJob::collectionsReceived);
    QVERIFY(spy.isValid());
    AKVERIFYEXEC(job);
    Collection::List list = job->collections();

    int count = 0;
    for (int i = 0; i < spy.count(); ++i) {
        auto l = spy[i][0].value<Akonadi::Collection::List>();
        for (int j = 0; j < l.count(); ++j) {
            QVERIFY(list.count() > count + j);
            QCOMPARE(list[count + j].id(), l[j].id());
        }
        count += l.count();
    }
    QCOMPARE(count, list.count());

    // check if everything is there
    QCOMPARE(list.count(), 4);
    Collection col;
    QStringList contentTypes;

    col = findCol(list, QStringLiteral("foo"));
    QVERIFY(col.isValid());
    QCOMPARE(col.parentCollection().id(), res1ColId);
    QVERIFY(CollectionUtils::isFolder(col));
    contentTypes << QStringLiteral("message/rfc822") << QStringLiteral("text/calendar") << QStringLiteral("text/directory")
                 << QStringLiteral("application/octet-stream") << QStringLiteral("inode/directory");
    compareLists(col.contentMimeTypes(), contentTypes);

    QVERIFY(findCol(list, QStringLiteral("bar")).isValid());
    QCOMPARE(findCol(list, QStringLiteral("bar")).parentCollection(), col);
    QVERIFY(findCol(list, QStringLiteral("bla")).isValid());
}

class ResultSignalTester : public QObject
{
    Q_OBJECT
public:
    QStringList receivedSignals;
public Q_SLOTS:
    void onCollectionsReceived(const Akonadi::Collection::List & /*unused*/)
    {
        receivedSignals << QStringLiteral("collectionsReceived");
    }

    void onCollectionRetrievalDone(KJob * /*unused*/)
    {
        receivedSignals << QStringLiteral("result");
    }
};

void CollectionJobTest::testSignalOrder()
{
    Akonadi::Collection::List toFetch;
    toFetch << Collection(res1ColId);
    toFetch << Collection(res2ColId);
    auto *job = new CollectionFetchJob(toFetch, CollectionFetchJob::Recursive);
    ResultSignalTester spy;
    connect(job, &CollectionFetchJob::collectionsReceived, &spy, &ResultSignalTester::onCollectionsReceived);
    connect(job, &KJob::result, &spy, &ResultSignalTester::onCollectionRetrievalDone);
    AKVERIFYEXEC(job);

    QCOMPARE(spy.receivedSignals.size(), 2);
    QCOMPARE(spy.receivedSignals.at(0), QStringLiteral("collectionsReceived"));
    QCOMPARE(spy.receivedSignals.at(1), QStringLiteral("result"));
}

void CollectionJobTest::testNonRecursiveFolderList()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection(res1ColId), CollectionFetchJob::Base);
    AKVERIFYEXEC(job);
    Collection::List list = job->collections();

    QCOMPARE(list.count(), 1);
    QVERIFY(findCol(list, QStringLiteral("res1")).isValid());
}

void CollectionJobTest::testEmptyFolderList()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection(res3ColId), CollectionFetchJob::FirstLevel);
    AKVERIFYEXEC(job);
    Collection::List list = job->collections();

    QCOMPARE(list.count(), 0);
}

void CollectionJobTest::testSearchFolderList()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection(searchColId), CollectionFetchJob::FirstLevel);
    AKVERIFYEXEC(job);
    Collection::List list = job->collections();

    QCOMPARE(list.count(), 0);
}

void CollectionJobTest::testResourceFolderList()
{
    // non-existing resource
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::FirstLevel);
    job->fetchScope().setResource(QStringLiteral("i_dont_exist"));
    QVERIFY(!job->exec());

    // recursive listing of all collections of an existing resource
    job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
    job->fetchScope().setResource(QStringLiteral("akonadi_knut_resource_0"));
    AKVERIFYEXEC(job);

    Collection::List list = job->collections();
    QCOMPARE(list.count(), 5);
    QVERIFY(findCol(list, QStringLiteral("res1")).isValid());
    QVERIFY(findCol(list, QStringLiteral("foo")).isValid());
    QVERIFY(findCol(list, QStringLiteral("bar")).isValid());
    QVERIFY(findCol(list, QStringLiteral("bla")).isValid());
    int fooId = findCol(list, QStringLiteral("foo")).id();

    // limited listing of a resource
    job = new CollectionFetchJob(Collection(fooId), CollectionFetchJob::Recursive);
    job->fetchScope().setResource(QStringLiteral("akonadi_knut_resource_0"));
    AKVERIFYEXEC(job);

    list = job->collections();
    QCOMPARE(list.count(), 3);
    QVERIFY(findCol(list, QStringLiteral("bar")).isValid());
    QVERIFY(findCol(list, QStringLiteral("bla")).isValid());
}

void CollectionJobTest::testMimeTypeFilter()
{
    CollectionFetchJob *job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive);
    job->fetchScope().setContentMimeTypes(QStringList() << QStringLiteral("message/rfc822"));
    AKVERIFYEXEC(job);

    Collection::List list = job->collections();
    QCOMPARE(list.count(), 2);
    QVERIFY(findCol(list, QStringLiteral("res1")).isValid());
    QVERIFY(findCol(list, QStringLiteral("foo")).isValid());
    int fooId = findCol(list, QStringLiteral("foo")).id();

    // limited listing of a resource
    job = new CollectionFetchJob(Collection(fooId), CollectionFetchJob::Recursive);
    job->fetchScope().setContentMimeTypes(QStringList() << QStringLiteral("message/rfc822"));
    AKVERIFYEXEC(job);

    list = job->collections();
    QCOMPARE(list.count(), 0);

    // non-existing mimetype
    job = new CollectionFetchJob(Collection::root(), CollectionFetchJob::Recursive, this);
    job->fetchScope().setContentMimeTypes(QStringList() << QStringLiteral("something/non-existing"));
    AKVERIFYEXEC(job);
    QCOMPARE(job->collections().size(), 0);
}

void CollectionJobTest::testCreateDeleteFolder_data()
{
    QTest::addColumn<Collection>("collection");
    QTest::addColumn<bool>("creatable");

    Collection col;
    QTest::newRow("empty") << col << false;
    col.setName(QStringLiteral("new folder"));
    col.parentCollection().setId(res3ColId);
    QTest::newRow("simple") << col << true;

    col.parentCollection().setId(res3ColId);
    col.setName(QStringLiteral("foo"));
    QTest::newRow("existing in different resource") << col << true;

    col.setName(QStringLiteral("mail folder"));
    QStringList mimeTypes;
    mimeTypes << QStringLiteral("inode/directory") << QStringLiteral("message/rfc822");
    col.setContentMimeTypes(mimeTypes);
    col.setRemoteId(QStringLiteral("remote id"));
    CachePolicy policy;
    policy.setInheritFromParent(false);
    policy.setIntervalCheckTime(60);
    policy.setLocalParts({QStringLiteral("PLD:ENVELOPE")});
    policy.setSyncOnDemand(true);
    policy.setCacheTimeout(120);
    col.setCachePolicy(policy);
    QTest::newRow("complex") << col << true;

    col = Collection();
    col.setName(QStringLiteral("New Folder"));
    col.parentCollection().setId(searchColId);
    QTest::newRow("search folder") << col << false;

    col.parentCollection().setId(res2ColId);
    col.setName(QStringLiteral("foo2"));
    QTest::newRow("already existing") << col << false;

    col.parentCollection().setId(res2ColId);   // Sibling of collection 'foo2'
    col.setName(QStringLiteral("foo2 "));
    QTest::newRow("name of an sibling with an additional ending space") << col << true;

    col.setName(QStringLiteral("Bla"));
    col.parentCollection().setId(2);
    QTest::newRow("already existing with different case") << col << true;

    CollectionPathResolver *resolver = new CollectionPathResolver(QStringLiteral("res2/foo2"), this);
    AKVERIFYEXEC(resolver);
    col.parentCollection().setId(resolver->collection());
    col.setName(QStringLiteral("new folder"));
    QTest::newRow("parent noinferior") << col << false;

    col.parentCollection().setId(INT_MAX);
    QTest::newRow("missing parent") << col << false;

    col = Collection();
    col.setName(QStringLiteral("rid parent"));
    col.parentCollection().setRemoteId(QStringLiteral("8"));
    QTest::newRow("rid parent") << col << false;   // missing resource context
}

void CollectionJobTest::testCreateDeleteFolder()
{
    QFETCH(Collection, collection);
    QFETCH(bool, creatable);

    auto *createJob = new CollectionCreateJob(collection, this);
    QCOMPARE(createJob->exec(), creatable);
    if (!creatable) {
        return;
    }

    Collection createdCol = createJob->collection();
    QVERIFY(createdCol.isValid());
    QCOMPARE(createdCol.name(), collection.name());
    QCOMPARE(createdCol.parentCollection(), collection.parentCollection());
    QCOMPARE(createdCol.remoteId(), collection.remoteId());
    QCOMPARE(createdCol.cachePolicy(), collection.cachePolicy());

    auto *listJob = new CollectionFetchJob(collection.parentCollection(), CollectionFetchJob::FirstLevel, this);
    AKVERIFYEXEC(listJob);
    Collection listedCol = findCol(listJob->collections(), collection.name());
    QCOMPARE(listedCol, createdCol);
    QCOMPARE(listedCol.remoteId(), collection.remoteId());
    QCOMPARE(listedCol.cachePolicy(), collection.cachePolicy());

    // fetch parent to compare inherited collection properties
    Collection parentCol = Collection::root();
    if (collection.parentCollection().isValid()) {
        auto *listJob = new CollectionFetchJob(collection.parentCollection(), CollectionFetchJob::Base, this);
        AKVERIFYEXEC(listJob);
        QCOMPARE(listJob->collections().count(), 1);
        parentCol = listJob->collections().first();
    }

    if (collection.contentMimeTypes().isEmpty()) {
        compareLists(listedCol.contentMimeTypes(), parentCol.contentMimeTypes());
    } else {
        compareLists(listedCol.contentMimeTypes(), collection.contentMimeTypes());
    }

    if (collection.resource().isEmpty()) {
        QCOMPARE(listedCol.resource(), parentCol.resource());
    } else {
        QCOMPARE(listedCol.resource(), collection.resource());
    }

    auto *delJob = new CollectionDeleteJob(createdCol, this);
    AKVERIFYEXEC(delJob);

    listJob = new CollectionFetchJob(collection.parentCollection(), CollectionFetchJob::FirstLevel, this);
    AKVERIFYEXEC(listJob);
    QVERIFY(!findCol(listJob->collections(), collection.name()).isValid());
}

void CollectionJobTest::testIllegalDeleteFolder()
{
    // non-existing folder
    CollectionDeleteJob *del = new CollectionDeleteJob(Collection(INT_MAX), this);
    QVERIFY(!del->exec());

    // root
    del = new CollectionDeleteJob(Collection::root(), this);
    QVERIFY(!del->exec());
}

void CollectionJobTest::testStatistics()
{
    // empty folder
    CollectionStatisticsJob *statistics =
        new CollectionStatisticsJob(Collection(res1ColId), this);
    AKVERIFYEXEC(statistics);

    CollectionStatistics s = statistics->statistics();
    QCOMPARE(s.count(), 0LL);
    QCOMPARE(s.unreadCount(), 0LL);

    // folder with attributes and content
    CollectionPathResolver *resolver = new CollectionPathResolver(QStringLiteral("res1/foo"), this);
    AKVERIFYEXEC(resolver);
    statistics = new CollectionStatisticsJob(Collection(resolver->collection()), this);
    AKVERIFYEXEC(statistics);

    s = statistics->statistics();
    QCOMPARE(s.count(), 15LL);
    QCOMPARE(s.unreadCount(), 14LL);
}

void CollectionJobTest::testModify_data()
{
    QTest::addColumn<qint64>("uid");
    QTest::addColumn<QString>("rid");

    QTest::newRow("uid") << AkonadiTest::collectionIdFromPath(QStringLiteral("res1/foo")) << QString();
    QTest::newRow("rid") << -1LL << QStringLiteral("10");
}

#define RESET_COLLECTION_ID \
    col.setId( uid ); \
    if ( !rid.isEmpty() ) col.setRemoteId( rid )

void CollectionJobTest::testModify()
{
    QFETCH(qint64, uid);
    QFETCH(QString, rid);

    if (!rid.isEmpty()) {
        ResourceSelectJob *rjob = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"));
        AKVERIFYEXEC(rjob);
    }

    const QStringList reference = { QStringLiteral("text/calendar"), QStringLiteral("text/directory"), QStringLiteral("message/rfc822"),
                                    QStringLiteral("application/octet-stream"), QStringLiteral("inode/directory")
                                  };

    Collection col;
    RESET_COLLECTION_ID;

    // test noop modify
    auto *mod = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(mod);

    auto *ljob = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(ljob);
    QCOMPARE(ljob->collections().count(), 1);
    col = ljob->collections().first();
    compareLists(col.contentMimeTypes(), reference);

    // test clearing content types
    RESET_COLLECTION_ID;
    col.setContentMimeTypes(QStringList());
    mod = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(mod);

    ljob = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(ljob);
    QCOMPARE(ljob->collections().count(), 1);
    col = ljob->collections().first();
    QVERIFY(col.contentMimeTypes().isEmpty());

    // test setting contnet types
    RESET_COLLECTION_ID;
    col.setContentMimeTypes(reference);
    mod = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(mod);

    ljob = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(ljob);
    QCOMPARE(ljob->collections().count(), 1);
    col = ljob->collections().first();
    compareLists(col.contentMimeTypes(), reference);

    // add attribute
    RESET_COLLECTION_ID;
    col.attribute<TestAttribute>(Collection::AddIfMissing)->data = "new";
    mod = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(mod);

    ljob = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(ljob);
    QVERIFY(ljob->collections().first().hasAttribute<TestAttribute>());
    QCOMPARE(ljob->collections().first().attribute<TestAttribute>()->data, QByteArray("new"));

    // modify existing attribute
    RESET_COLLECTION_ID;
    col.attribute<TestAttribute>()->data = "modified";
    mod = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(mod);

    ljob = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(ljob);
    QVERIFY(ljob->collections().first().hasAttribute<TestAttribute>());
    QCOMPARE(ljob->collections().first().attribute<TestAttribute>()->data, QByteArray("modified"));

    // renaming
    RESET_COLLECTION_ID;
    col.setName(QStringLiteral("foo (renamed)"));
    mod = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(mod);

    ljob = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(ljob);
    QCOMPARE(ljob->collections().count(), 1);
    col = ljob->collections().first();
    QCOMPARE(col.name(), QStringLiteral("foo (renamed)"));

    RESET_COLLECTION_ID;
    col.setName(QStringLiteral("foo"));
    mod = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(mod);
}

#undef RESET_COLLECTION_ID

void CollectionJobTest::testIllegalModify()
{
    // non-existing collection
    Collection col(INT_MAX);
    col.parentCollection().setId(res1ColId);
    auto *mod = new CollectionModifyJob(col, this);
    QVERIFY(!mod->exec());

    // rename to already existing name
    col = Collection(res1ColId);
    col.setName(QStringLiteral("res2"));
    mod = new CollectionModifyJob(col, this);
    QVERIFY(!mod->exec());
}

void CollectionJobTest::testUtf8CollectionName_data()
{
    QTest::addColumn<QString>("folderName");

    QTest::newRow("Umlaut") << QString::fromUtf8("ä");
    QTest::newRow("Garbage") << QString::fromUtf8("đ→³}đþøæſð");
    QTest::newRow("Utf8") << QString::fromUtf8("日本語");
}

void CollectionJobTest::testUtf8CollectionName()
{
    QFETCH(QString, folderName);

    // create collection
    Collection col;
    col.parentCollection().setId(res3ColId);
    col.setName(folderName);
    auto *create = new CollectionCreateJob(col, this);
    AKVERIFYEXEC(create);
    col = create->collection();
    QVERIFY(col.isValid());
    QCOMPARE(col.name(), folderName);

    // list parent
    CollectionFetchJob *list = new CollectionFetchJob(Collection(res3ColId), CollectionFetchJob::Recursive, this);
    AKVERIFYEXEC(list);
    QCOMPARE(list->collections().count(), 1);
    QCOMPARE(list->collections().first(), col);
    QCOMPARE(list->collections().first().name(), col.name());

    // modify collection
    col.setContentMimeTypes( { QStringLiteral("message/rfc822") } );
    auto *modify = new CollectionModifyJob(col, this);
    AKVERIFYEXEC(modify);

    // collection statistics
    auto *statistics = new CollectionStatisticsJob(col, this);
    AKVERIFYEXEC(statistics);
    CollectionStatistics s = statistics->statistics();
    QCOMPARE(s.count(), 0LL);
    QCOMPARE(s.unreadCount(), 0LL);

    // delete collection
    auto *del = new CollectionDeleteJob(col, this);
    AKVERIFYEXEC(del);
}

void CollectionJobTest::testMultiList()
{
    Collection::List req;
    req << Collection(res1ColId) << Collection(res2ColId);
    auto *job = new CollectionFetchJob(req, this);
    AKVERIFYEXEC(job);

    Collection::List res;
    res = job->collections();
    compareLists(res, req);
}

void CollectionJobTest::testMultiListInvalid()
{
    Collection::List req;
    req << Collection(res1ColId) << Collection(1234567) << Collection(res2ColId);
    auto *job = new CollectionFetchJob(req, this);
    QVERIFY(!job->exec());
    // not all available collections are fetched
    QVERIFY(job->collections().count() != 2);

    job = new CollectionFetchJob(req, this);
    job->fetchScope().setIgnoreRetrievalErrors(true);
    QVERIFY(!job->exec());
    Collection::List res;
    res = job->collections();
    req = Collection::List() << Collection(res1ColId) << Collection(res2ColId);
    compareLists(res, req);
}

void CollectionJobTest::testRecursiveMultiList()
{
    Akonadi::Collection::List toFetch;
    toFetch << Collection(res1ColId);
    toFetch << Collection(res2ColId);
    auto *job = new CollectionFetchJob(toFetch, CollectionFetchJob::Recursive);
    QSignalSpy spy(job, &CollectionFetchJob::collectionsReceived);
    QVERIFY(spy.isValid());
    AKVERIFYEXEC(job);

    Collection::List list = job->collections();

    int count = 0;
    for (int i = 0; i < spy.count(); ++i) {
        auto l = spy[i][0].value<Akonadi::Collection::List>();
        for (int j = 0; j < l.count(); ++j) {
            QVERIFY(list.count() > count + j);
            QCOMPARE(list[count + j].id(), l[j].id());
        }
        count += l.count();
    }
    QCOMPARE(count, list.count());

    // check if everything is there
    QCOMPARE(list.count(), 4 + 2);
    QVERIFY(findCol(list, QStringLiteral("foo")).isValid());
    QVERIFY(findCol(list, QStringLiteral("bar")).isValid());
    QVERIFY(findCol(list, QStringLiteral("bla")).isValid());     //There are two bla folders, but we only check for one.
    QVERIFY(findCol(list, QStringLiteral("foo2")).isValid());
    QVERIFY(findCol(list, QStringLiteral("space folder")).isValid());
}

void CollectionJobTest::testNonOverlappingRootList()
{
    Akonadi::Collection::List toFetch;
    toFetch << Collection(res1ColId);
    toFetch << Collection(res2ColId);
    auto *job = new CollectionFetchJob(toFetch, CollectionFetchJob::NonOverlappingRoots);
    QSignalSpy spy(job, &CollectionFetchJob::collectionsReceived);
    QVERIFY(spy.isValid());
    AKVERIFYEXEC(job);

    Collection::List list = job->collections();

    int count = 0;
    for (int i = 0; i < spy.count(); ++i) {
        auto l = spy[i][0].value<Akonadi::Collection::List>();
        for (int j = 0; j < l.count(); ++j) {
            QVERIFY(list.count() > count + j);
            QCOMPARE(list[count + j].id(), l[j].id());
        }
        count += l.count();
    }
    QCOMPARE(count, list.count());

    // check if everything is there
    QCOMPARE(list.count(), 2);
    QVERIFY(findCol(list, QStringLiteral("res1")).isValid());
    QVERIFY(findCol(list, QStringLiteral("res2")).isValid());
}

void CollectionJobTest::testRidFetch()
{
    Collection col;
    col.setRemoteId(QStringLiteral("10"));

    auto *job = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    job->fetchScope().setResource(QStringLiteral("akonadi_knut_resource_0"));
    AKVERIFYEXEC(job);
    QCOMPARE(job->collections().count(), 1);
    col = job->collections().first();
    QVERIFY(col.isValid());
    QCOMPARE(col.remoteId(), QString::fromLatin1("10"));
}

void CollectionJobTest::testRidCreateDelete_data()
{
    QTest::addColumn<QString>("remoteId");
    QTest::newRow("ASCII") << QString::fromUtf8("MY REMOTE ID");
    QTest::newRow("LATIN1") << QString::fromUtf8("MY REMÖTE ID");
    QTest::newRow("UTF8") << QString::fromUtf8("MY REMOTE 検索表");
}

void CollectionJobTest::testRidCreateDelete()
{
    QFETCH(QString, remoteId);
    Collection collection;
    collection.setName(QStringLiteral("rid create"));
    collection.parentCollection().setRemoteId(QStringLiteral("8"));
    collection.setRemoteId(remoteId);

    ResourceSelectJob *resSel = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_2"));
    AKVERIFYEXEC(resSel);

    auto *createJob = new CollectionCreateJob(collection, this);
    AKVERIFYEXEC(createJob);

    Collection createdCol = createJob->collection();
    QVERIFY(createdCol.isValid());
    QCOMPARE(createdCol.name(), collection.name());

    CollectionFetchJob *listJob = new CollectionFetchJob(Collection(res3ColId), CollectionFetchJob::FirstLevel, this);
    AKVERIFYEXEC(listJob);
    Collection listedCol = findCol(listJob->collections(), collection.name());
    QCOMPARE(listedCol, createdCol);
    QCOMPARE(listedCol.name(), collection.name());

    QVERIFY(!collection.isValid());
    auto *delJob = new CollectionDeleteJob(collection, this);
    AKVERIFYEXEC(delJob);

    listJob = new CollectionFetchJob(Collection(res3ColId), CollectionFetchJob::FirstLevel, this);
    AKVERIFYEXEC(listJob);
    QVERIFY(!findCol(listJob->collections(), collection.name()).isValid());
}

void CollectionJobTest::testAncestorRetrieval()
{
    Collection col;
    col.setRemoteId(QStringLiteral("10"));

    auto *job = new CollectionFetchJob(col, CollectionFetchJob::Base, this);
    job->fetchScope().setResource(QStringLiteral("akonadi_knut_resource_0"));
    job->fetchScope().setAncestorRetrieval(CollectionFetchScope::All);
    AKVERIFYEXEC(job);
    QCOMPARE(job->collections().count(), 1);
    col = job->collections().first();
    QVERIFY(col.isValid());
    QVERIFY(col.parentCollection().isValid());
    QCOMPARE(col.parentCollection().remoteId(), QStringLiteral("6"));
    QCOMPARE(col.parentCollection().parentCollection(), Collection::root());

    ResourceSelectJob *select = new ResourceSelectJob(QStringLiteral("akonadi_knut_resource_0"), this);
    AKVERIFYEXEC(select);
    Collection col2(col);
    col2.setId(-1);   // make it invalid but keep the ancestor chain
    job = new CollectionFetchJob(col2, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(job);
    QCOMPARE(job->collections().count(), 1);
    col2 = job->collections().first();
    QVERIFY(col2.isValid());
    QCOMPARE(col, col2);
}

void CollectionJobTest::testAncestorAttributeRetrieval()
{
    Akonadi::Collection baseCol;
    {
        baseCol.setParentCollection(Akonadi::Collection(res1ColId));
        baseCol.setName(QStringLiteral("base"));
        baseCol.attribute<TestAttribute>(Collection::AddIfMissing)->data = "new";
        auto *create = new Akonadi::CollectionCreateJob(baseCol);
        AKVERIFYEXEC(create);
        baseCol = create->collection();
    }
    {
        Akonadi::Collection col;
        col.setParentCollection(baseCol);
        col.setName(QStringLiteral("enabled"));
        auto *create = new Akonadi::CollectionCreateJob(col);
        AKVERIFYEXEC(create);

        CollectionFetchJob *job = new CollectionFetchJob(create->collection(), CollectionFetchJob::Base);
        job->fetchScope().setAncestorRetrieval(CollectionFetchScope::All);
        job->fetchScope().ancestorFetchScope().setFetchIdOnly(false);
        job->fetchScope().ancestorFetchScope().fetchAttribute<TestAttribute>();
        AKVERIFYEXEC(job);
        Akonadi::Collection result = job->collections().first();
        QCOMPARE(result.parentCollection().hasAttribute<TestAttribute>(), true);
    }

    //Cleanup
    auto *deleteJob = new CollectionDeleteJob(baseCol);
    AKVERIFYEXEC(deleteJob);
}

void CollectionJobTest::testListPreference()
{
    Akonadi::Collection baseCol;
    {
        baseCol.setParentCollection(Akonadi::Collection(res1ColId));
        baseCol.setName(QStringLiteral("base"));
        auto *create = new Akonadi::CollectionCreateJob(baseCol);
        AKVERIFYEXEC(create);
        baseCol = create->collection();
    }
    {
        Akonadi::Collection col;
        col.setParentCollection(baseCol);
        col.setEnabled(true);
        col.setName(QStringLiteral("enabled"));
        auto *create = new Akonadi::CollectionCreateJob(col);
        AKVERIFYEXEC(create);

        CollectionFetchJob *job = new CollectionFetchJob(create->collection(), CollectionFetchJob::Base);
        AKVERIFYEXEC(job);
        Akonadi::Collection result = job->collections().first();
        QCOMPARE(result.enabled(), true);
        QCOMPARE(result.localListPreference(Collection::ListDisplay), Collection::ListDefault);
        QCOMPARE(result.localListPreference(Collection::ListSync), Collection::ListDefault);
        QCOMPARE(result.localListPreference(Collection::ListIndex), Collection::ListDefault);
    }
    {
        Akonadi::Collection col;
        col.setParentCollection(baseCol);
        col.setName(QStringLiteral("disabledPref"));
        col.setEnabled(true);
        col.setLocalListPreference(Collection::ListDisplay, Collection::ListDisabled);
        col.setLocalListPreference(Collection::ListSync, Collection::ListDisabled);
        col.setLocalListPreference(Collection::ListIndex, Collection::ListDisabled);
        auto *create = new Akonadi::CollectionCreateJob(col);
        AKVERIFYEXEC(create);
        CollectionFetchJob *job = new CollectionFetchJob(create->collection(), CollectionFetchJob::Base);
        AKVERIFYEXEC(job);
        Akonadi::Collection result = job->collections().first();
        QCOMPARE(result.enabled(), true);
        QCOMPARE(result.localListPreference(Collection::ListDisplay), Collection::ListDisabled);
        QCOMPARE(result.localListPreference(Collection::ListSync), Collection::ListDisabled);
        QCOMPARE(result.localListPreference(Collection::ListIndex), Collection::ListDisabled);
    }
    {
        Akonadi::Collection col;
        col.setParentCollection(baseCol);
        col.setName(QStringLiteral("enabledPref"));
        col.setEnabled(false);
        col.setLocalListPreference(Collection::ListDisplay, Collection::ListEnabled);
        col.setLocalListPreference(Collection::ListSync, Collection::ListEnabled);
        col.setLocalListPreference(Collection::ListIndex, Collection::ListEnabled);
        auto *create = new Akonadi::CollectionCreateJob(col);
        AKVERIFYEXEC(create);
        CollectionFetchJob *job = new CollectionFetchJob(create->collection(), CollectionFetchJob::Base);
        AKVERIFYEXEC(job);
        Akonadi::Collection result = job->collections().first();
        QCOMPARE(result.enabled(), false);
        QCOMPARE(result.localListPreference(Collection::ListDisplay), Collection::ListEnabled);
        QCOMPARE(result.localListPreference(Collection::ListSync), Collection::ListEnabled);
        QCOMPARE(result.localListPreference(Collection::ListIndex), Collection::ListEnabled);
    }

    //Check list filter
    {
        auto *job = new CollectionFetchJob(baseCol, CollectionFetchJob::FirstLevel);
        job->fetchScope().setListFilter(CollectionFetchScope::Display);
        AKVERIFYEXEC(job);
        QCOMPARE(job->collections().size(), 2);
    }
    {
        auto *job = new CollectionFetchJob(baseCol, CollectionFetchJob::FirstLevel);
        job->fetchScope().setListFilter(CollectionFetchScope::Sync);
        AKVERIFYEXEC(job);
        QCOMPARE(job->collections().size(), 2);
    }
    {
        auto *job = new CollectionFetchJob(baseCol, CollectionFetchJob::FirstLevel);
        job->fetchScope().setListFilter(CollectionFetchScope::Index);
        AKVERIFYEXEC(job);
        QCOMPARE(job->collections().size(), 2);
    }
    {
        auto *job = new CollectionFetchJob(baseCol, CollectionFetchJob::FirstLevel);
        job->fetchScope().setListFilter(CollectionFetchScope::Enabled);
        AKVERIFYEXEC(job);
        QCOMPARE(job->collections().size(), 2);
    }

    //Cleanup
    auto *deleteJob = new CollectionDeleteJob(baseCol);
    AKVERIFYEXEC(deleteJob);
}

#include "collectionjobtest.moc"
