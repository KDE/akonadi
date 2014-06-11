/*
    Copyright 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "localfolderstest.h"

#include "../../collectionpathresolver_p.h"
#include "../../dbusconnectionpool.h"
#include "../specialmailcollectionssettings.h"

#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QFile>
#include <QSignalSpy>

#include <KDebug>
#include <KStandardDirs>

#include <akonadi/agentinstance.h>
#include <akonadi/agentmanager.h>
#include <akonadi/collectioncreatejob.h>
#include <akonadi/collectiondeletejob.h>
#include <akonadi/collectionfetchjob.h>
#include <akonadi/collectionmodifyjob.h>
#include <akonadi/control.h>
#include <akonadi/servermanager.h>
#include <akonadi/qtest_akonadi.h>
#include "../../specialcollectionattribute_p.h"
#include "../../specialcollections_p.h"
#include <akonadi/kmime/specialmailcollections.h>
#include "../specialmailcollectionstesting_p.h"
#include "../../specialcollectionshelperjobs_p.h"

using namespace Akonadi;

typedef SpecialMailCollectionsSettings Settings;

static Collection res1;

void LocalFoldersTest::initTestCase()
{
    qRegisterMetaType<Akonadi::AgentInstance>("Akonadi::AgentInstance");

    mDisplayNameMap.insert("local-mail", i18nc("local mail folder", "Local Folders"));
    mDisplayNameMap.insert("inbox", i18nc("local mail folder", "inbox"));
    mDisplayNameMap.insert("outbox", i18nc("local mail folder", "outbox"));
    mDisplayNameMap.insert("sent-mail", i18nc("local mail folder", "sent-mail"));
    mDisplayNameMap.insert("trash", i18nc("local mail folder", "trash"));
    mDisplayNameMap.insert("drafts", i18nc("local mail folder", "drafts"));
    mDisplayNameMap.insert("templates", i18nc("local mail folder", "templates"));

    mIconNameMap.insert("local-mail", QLatin1String("folder"));
    mIconNameMap.insert("inbox", QLatin1String("mail-folder-inbox"));
    mIconNameMap.insert("outbox", QLatin1String("mail-folder-outbox"));
    mIconNameMap.insert("sent-mail", QLatin1String("mail-folder-sent"));
    mIconNameMap.insert("trash", QLatin1String("user-trash"));
    mIconNameMap.insert("drafts", QLatin1String("document-properties"));
    mIconNameMap.insert("templates", QLatin1String("document-new"));

    QVERIFY(Control::start());
    QTest::qWait(1000);

    CollectionPathResolver *resolver = new CollectionPathResolver("res1", this);
    QVERIFY(resolver->exec());
    res1 = Collection(resolver->collection());

    CollectionFetchJob *fjob = new CollectionFetchJob(res1, CollectionFetchJob::Base, this);
    AKVERIFYEXEC(fjob);
    Q_ASSERT(fjob->collections().count() == 1);
    res1 = fjob->collections().first();
    QVERIFY(!res1.resource().isEmpty());
}

void LocalFoldersTest::testLock()
{
    QString dbusName = QString::fromLatin1("org.kde.pim.SpecialCollections");
    if (ServerManager::hasInstanceIdentifier()) {
      dbusName += Akonadi::ServerManager::instanceIdentifier();
    }


    // Initially not locked.
    QVERIFY(!DBusConnectionPool::threadConnection().interface()->isServiceRegistered(dbusName));

    // Get the lock.
    {
        GetLockJob *ljob = new GetLockJob(this);
        AKVERIFYEXEC(ljob);
        QVERIFY(DBusConnectionPool::threadConnection().interface()->isServiceRegistered(dbusName));
    }

    // Getting the lock again should fail.
    {
        GetLockJob *ljob = new GetLockJob(this);
        QVERIFY(!ljob->exec());
    }

    // Release the lock.
    QVERIFY(DBusConnectionPool::threadConnection().interface()->isServiceRegistered(dbusName));
    releaseLock();
    QVERIFY(!DBusConnectionPool::threadConnection().interface()->isServiceRegistered(dbusName));
}

void LocalFoldersTest::testInitialState()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialCollections *sc = smc;
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    Q_UNUSED(smct);

    // No one has created the default resource.
    QVERIFY(sc->d->defaultResource().identifier().isEmpty());

    // LF does not have a default Outbox folder (or any other).
    QCOMPARE(smc->hasDefaultCollection(SpecialMailCollections::Outbox), false);
    QVERIFY(!smc->defaultCollection(SpecialMailCollections::Outbox).isValid());

    // LF treats unknown resources correctly.
    const QString resourceId = QString::fromLatin1("this_resource_does_not_exist");
    QCOMPARE(smc->hasCollection(SpecialMailCollections::Outbox, AgentManager::self()->instance(resourceId)), false);
    QVERIFY(!smc->collection(SpecialMailCollections::Outbox, AgentManager::self()->instance(resourceId)).isValid());
}

void LocalFoldersTest::testRegistrationErrors()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();

    // A valid collection that can be registered.
    Collection outbox;
    outbox.setName(QLatin1String("my_outbox"));
    outbox.setParentCollection(res1);
    outbox.addAttribute(new SpecialCollectionAttribute("outbox"));
    outbox.setResource(res1.resource());

    // Needs to be valid.
    {
        Collection col(outbox);
        col.setId(-1);
        QVERIFY(!smc->registerCollection(SpecialMailCollections::Outbox, col));
    }

    // Needs to have a resourceId.
    {
        Collection col(outbox);
        col.setResource(QString());
        QVERIFY(!smc->registerCollection(SpecialMailCollections::Outbox, col));
    }

    // Needs to have a LocalFolderAttribute.
    {
        Collection col(outbox);
        col.removeAttribute<SpecialCollectionAttribute>();
        QVERIFY(!smc->registerCollection(SpecialMailCollections::Outbox, col));
    }
}

void LocalFoldersTest::testDefaultFolderRegistration()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialCollections *sc = smc;
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    QSignalSpy spy(smc, SIGNAL(collectionsChanged(Akonadi::AgentInstance)));
    QSignalSpy defSpy(smc, SIGNAL(defaultCollectionsChanged()));
    QVERIFY(spy.isValid());
    QVERIFY(defSpy.isValid());

    // No one has created the default resource.
    QVERIFY(sc->d->defaultResource().identifier().isEmpty());

    // Set the default resource. LF should still have no folders.
    smct->_t_setDefaultResourceId(res1.resource());
    QCOMPARE(sc->d->defaultResource().identifier(), res1.resource());
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);

    // Manually create an Outbox collection.
    Collection outbox;
    outbox.setName(QLatin1String("my_outbox"));
    outbox.setParentCollection(res1);
    outbox.addAttribute(new SpecialCollectionAttribute("outbox"));
    CollectionCreateJob *cjob = new CollectionCreateJob(outbox, this);
    AKVERIFYEXEC(cjob);
    outbox = cjob->collection();

    // LF should still have no folders, since the Outbox wasn't registered.
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(defSpy.count(), 0);

    // Register the collection. LF should now know it.
    bool ok = smc->registerCollection(SpecialMailCollections::Outbox, outbox);
    QVERIFY(ok);
    QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
    QCOMPARE(smc->defaultCollection(SpecialMailCollections::Outbox), outbox);
    QCOMPARE(smct->_t_knownResourceCount(), 1);
    QCOMPARE(smct->_t_knownFolderCount(), 1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(defSpy.count(), 1);

    // Forget all folders in the default resource.
    smct->_t_forgetFoldersForResource(sc->d->defaultResource().identifier());
    QCOMPARE(smc->hasDefaultCollection(SpecialMailCollections::Outbox), false);
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(defSpy.count(), 2);

    // Delete the collection.
    CollectionDeleteJob *djob = new CollectionDeleteJob(outbox, this);
    AKVERIFYEXEC(djob);
}

void LocalFoldersTest::testCustomFolderRegistration()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    QSignalSpy spy(smc, SIGNAL(collectionsChanged(Akonadi::AgentInstance)));
    QSignalSpy defSpy(smc, SIGNAL(defaultCollectionsChanged()));
    QVERIFY(spy.isValid());
    QVERIFY(defSpy.isValid());

    // Set a fake default resource, so that res1.resource() isn't seen as the default one.
    // LF should have no folders.
    smct->_t_setDefaultResourceId("dummy");
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);

    // Manually create an Outbox collection.
    Collection outbox;
    outbox.setName(QLatin1String("my_outbox"));
    outbox.setParentCollection(res1);
    outbox.addAttribute(new SpecialCollectionAttribute("outbox"));
    CollectionCreateJob *cjob = new CollectionCreateJob(outbox, this);
    AKVERIFYEXEC(cjob);
    outbox = cjob->collection();

    // LF should still have no folders, since the Outbox wasn't registered.
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(defSpy.count(), 0);

    // Register the collection. LF should now know it.
    bool ok = smc->registerCollection(SpecialMailCollections::Outbox, outbox);
    QVERIFY(ok);
    QVERIFY(!smc->hasDefaultCollection(SpecialMailCollections::Outbox));
    QVERIFY(smc->hasCollection(SpecialMailCollections::Outbox, AgentManager::self()->instance(outbox.resource())));
    QCOMPARE(smc->collection(SpecialMailCollections::Outbox, AgentManager::self()->instance(outbox.resource())), outbox);
    QCOMPARE(smct->_t_knownResourceCount(), 1);
    QCOMPARE(smct->_t_knownFolderCount(), 1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(defSpy.count(), 0);

    // Forget all folders in this resource.
    smct->_t_forgetFoldersForResource(outbox.resource());
    QCOMPARE(smc->hasDefaultCollection(SpecialMailCollections::Outbox), false);
    QCOMPARE(smc->hasCollection(SpecialMailCollections::Outbox, AgentManager::self()->instance(outbox.resource())), false);
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(defSpy.count(), 0);

    // Delete the collection.
    CollectionDeleteJob *djob = new CollectionDeleteJob(outbox, this);
    AKVERIFYEXEC(djob);
}

void LocalFoldersTest::testCollectionDelete()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialCollections *sc = smc;
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    QSignalSpy spy(smc, SIGNAL(collectionsChanged(Akonadi::AgentInstance)));
    QSignalSpy defSpy(smc, SIGNAL(defaultCollectionsChanged()));
    QVERIFY(spy.isValid());
    QVERIFY(defSpy.isValid());

    // Set the default resource. LF should have no folders.
    smct->_t_setDefaultResourceId(res1.resource());
    QCOMPARE(sc->d->defaultResource().identifier(), res1.resource());
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);

    // Manually create an Outbox collection.
    Collection outbox;
    outbox.setName(QLatin1String("my_outbox"));
    outbox.setParentCollection(res1);
    outbox.addAttribute(new SpecialCollectionAttribute("outbox"));
    CollectionCreateJob *cjob = new CollectionCreateJob(outbox, this);
    AKVERIFYEXEC(cjob);
    outbox = cjob->collection();

    // LF should still have no folders, since the Outbox wasn't registered.
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(defSpy.count(), 0);

    // Register the collection. LF should now know it.
    bool ok = smc->registerCollection(SpecialMailCollections::Outbox, outbox);
    QVERIFY(ok);
    QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
    QCOMPARE(smc->defaultCollection(SpecialMailCollections::Outbox), outbox);
    QCOMPARE(smct->_t_knownResourceCount(), 1);
    QCOMPARE(smct->_t_knownFolderCount(), 1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(defSpy.count(), 1);

    // Delete the collection. LF should watch for that.
    CollectionDeleteJob *djob = new CollectionDeleteJob(outbox, this);
    AKVERIFYEXEC(djob);
    QTest::qWait(100);
    QVERIFY(!smc->hasDefaultCollection(SpecialMailCollections::Outbox));
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(defSpy.count(), 2);
}

void LocalFoldersTest::testBatchRegister()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialCollections *sc = smc;
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    QSignalSpy spy(smc, SIGNAL(collectionsChanged(Akonadi::AgentInstance)));
    QSignalSpy defSpy(smc, SIGNAL(defaultCollectionsChanged()));
    QVERIFY(spy.isValid());
    QVERIFY(defSpy.isValid());

    // Set the default resource. LF should have no folders.
    smct->_t_setDefaultResourceId(res1.resource());
    QCOMPARE(sc->d->defaultResource().identifier(), res1.resource());
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);

    // Manually create an Outbox collection.
    Collection outbox;
    outbox.setName(QLatin1String("my_outbox"));
    outbox.setParentCollection(res1);
    outbox.addAttribute(new SpecialCollectionAttribute("outbox"));
    CollectionCreateJob *cjob = new CollectionCreateJob(outbox, this);
    AKVERIFYEXEC(cjob);
    outbox = cjob->collection();

    // Manually create a Drafts collection.
    Collection drafts;
    drafts.setName(QLatin1String("my_drafts"));
    drafts.setParentCollection(res1);
    drafts.addAttribute(new SpecialCollectionAttribute("drafts"));
    cjob = new CollectionCreateJob(drafts, this);
    AKVERIFYEXEC(cjob);
    drafts = cjob->collection();

    // LF should still have no folders, since the folders were not registered.
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(defSpy.count(), 0);

    // Register the folders in batch mode.
    smct->_t_beginBatchRegister();
    bool ok = smc->registerCollection(SpecialMailCollections::Outbox, outbox);
    QVERIFY(ok);
    QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
    QCOMPARE(smc->defaultCollection(SpecialMailCollections::Outbox), outbox);
    QCOMPARE(smct->_t_knownResourceCount(), 1);
    QCOMPARE(smct->_t_knownFolderCount(), 1);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(defSpy.count(), 0);
    ok = smc->registerCollection(SpecialMailCollections::Drafts, drafts);
    QVERIFY(ok);
    QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Drafts));
    QCOMPARE(smc->defaultCollection(SpecialMailCollections::Drafts), drafts);
    QCOMPARE(smct->_t_knownResourceCount(), 1);
    QCOMPARE(smct->_t_knownFolderCount(), 2);
    QCOMPARE(spy.count(), 0);
    QCOMPARE(defSpy.count(), 0);
    smct->_t_endBatchRegister();
    QCOMPARE(smct->_t_knownResourceCount(), 1);
    QCOMPARE(smct->_t_knownFolderCount(), 2);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(defSpy.count(), 1);

    // Forget all folders in the default resource.
    smct->_t_forgetFoldersForResource(sc->d->defaultResource().identifier());
    QCOMPARE(smc->hasDefaultCollection(SpecialMailCollections::Outbox), false);
    QCOMPARE(smc->hasDefaultCollection(SpecialMailCollections::Drafts), false);
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(defSpy.count(), 2);

    // Delete the collections.
    CollectionDeleteJob *djob = new CollectionDeleteJob(outbox, this);
    AKVERIFYEXEC(djob);
    djob = new CollectionDeleteJob(drafts, this);
    AKVERIFYEXEC(djob);
}

void LocalFoldersTest::testResourceScanErrors()
{
    // Job fails if no resourceId is given.
    ResourceScanJob *resjob = new ResourceScanJob(QString(), Settings::self(), this);
    QVERIFY(!resjob->exec());
}

void LocalFoldersTest::testResourceScan()
{
    // Verify that res1 has no collections.
    {
        CollectionFetchJob *fjob = new CollectionFetchJob(res1, CollectionFetchJob::Recursive, this);
        AKVERIFYEXEC(fjob);
        QCOMPARE(fjob->collections().count(), 0);
    }

    // Manually create an Outbox collection.
    Collection outbox;
    {
        outbox.setName(QLatin1String("my_outbox"));
        outbox.setParentCollection(res1);
        outbox.addAttribute(new SpecialCollectionAttribute("outbox"));
        CollectionCreateJob *cjob = new CollectionCreateJob(outbox, this);
        AKVERIFYEXEC(cjob);
        outbox = cjob->collection();
    }

    // Manually create a Drafts collection.
    Collection drafts;
    {
        drafts.setName(QLatin1String("my_drafts"));
        drafts.setParentCollection(res1);
        drafts.addAttribute(new SpecialCollectionAttribute("drafts"));
        CollectionCreateJob *cjob = new CollectionCreateJob(drafts, this);
        AKVERIFYEXEC(cjob);
        drafts = cjob->collection();
    }

    // Manually create a non-LocalFolder collection.
    Collection intruder;
    {
        intruder.setName(QLatin1String("intruder"));
        intruder.setParentCollection(res1);
        CollectionCreateJob *cjob = new CollectionCreateJob(intruder, this);
        AKVERIFYEXEC(cjob);
        intruder = cjob->collection();
    }

    // Verify that the collections have been created properly.
    {
        CollectionFetchJob *fjob = new CollectionFetchJob(res1, CollectionFetchJob::Recursive, this);
        AKVERIFYEXEC(fjob);
        QCOMPARE(fjob->collections().count(), 3);
    }

    // Test that ResourceScanJob finds everything correctly.
    ResourceScanJob *resjob = new ResourceScanJob(res1.resource(), Settings::self(), this);
    AKVERIFYEXEC(resjob);
    QCOMPARE(resjob->rootResourceCollection(), res1);
    const Collection::List folders = resjob->specialCollections();
    QCOMPARE(folders.count(), 2);
    QVERIFY(folders.contains(outbox));
    QVERIFY(folders.contains(drafts));

    // Clean up after ourselves.
    CollectionDeleteJob *djob = new CollectionDeleteJob(outbox, this);
    AKVERIFYEXEC(djob);
    djob = new CollectionDeleteJob(drafts, this);
    AKVERIFYEXEC(djob);
    djob = new CollectionDeleteJob(intruder, this);
    AKVERIFYEXEC(djob);
}

void LocalFoldersTest::testDefaultResourceJob()
{
    // Initially the defaut maildir does not exist.
    QVERIFY(!QFile::exists(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));

    // Run the job.
    Collection maildirRoot;
    QString resourceId;
    {
        DefaultResourceJob *resjob = new DefaultResourceJob(Settings::self(), this);
        resjob->setDefaultResourceType(QLatin1String("akonadi_maildir_resource"));

        QVariantMap options;
        options.insert(QLatin1String("Name"), i18nc("local mail folder", "Local Folders"));
        options.insert(QLatin1String("Path"), QString(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));
        resjob->setDefaultResourceOptions(options);
        resjob->setTypes(mDisplayNameMap.keys());
        resjob->setNameForTypeMap(mDisplayNameMap);
        resjob->setIconForTypeMap(mIconNameMap);

        AKVERIFYEXEC(resjob);
        resourceId = resjob->resourceId();
        const Collection::List folders = resjob->specialCollections();
        QCOMPARE(folders.count(), 1);
        maildirRoot = folders.first();
        QVERIFY(maildirRoot.hasAttribute<SpecialCollectionAttribute>());
        QCOMPARE(maildirRoot.attribute<SpecialCollectionAttribute>()->collectionType(), QByteArray("local-mail"));
    }

    // The maildir should exist now.
    QVERIFY(QFile::exists(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));

    // Create a LocalFolder in the default resource.
    Collection outbox;
    {
        outbox.setName(QLatin1String("outbox"));
        outbox.setParentCollection(maildirRoot);
        outbox.addAttribute(new SpecialCollectionAttribute("outbox"));
        CollectionCreateJob *cjob = new CollectionCreateJob(outbox, this);
        AKVERIFYEXEC(cjob);
        outbox = cjob->collection();

        // Wait for the maildir resource to actually create the folder on disk...
        // (needed in testRecoverDefaultResource())
        QTest::qWait(500);
    }

    // Run the job again.
    {
        DefaultResourceJob *resjob = new DefaultResourceJob(Settings::self(), this);
        resjob->setDefaultResourceType(QLatin1String("akonadi_maildir_resource"));

        QVariantMap options;
        options.insert(QLatin1String("Name"), i18nc("local mail folder", "Local Folders"));
        options.insert(QLatin1String("Path"), QString(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));
        resjob->setDefaultResourceOptions(options);
        resjob->setTypes(mDisplayNameMap.keys());
        resjob->setNameForTypeMap(mDisplayNameMap);
        resjob->setIconForTypeMap(mIconNameMap);

        AKVERIFYEXEC(resjob);
        QCOMPARE(resourceId, resjob->resourceId());   // Did not mistakenly create another resource.
        const Collection::List folders = resjob->specialCollections();
        QCOMPARE(folders.count(), 2);
        QVERIFY(folders.contains(maildirRoot));
        QVERIFY(folders.contains(outbox));
    }
}

void LocalFoldersTest::testRecoverDefaultResource()
{
    // The maildirs should exist (created in testDefaultResourceJob).
    const QString xdgPath = KGlobal::dirs()->localxdgdatadir();
    const QString rootPath = xdgPath + QLatin1String("local-mail");
    const QString outboxPath = xdgPath + QString::fromLatin1(".%1.directory/%2") \
                               .arg(QLatin1String("local-mail"))
                               .arg(QLatin1String("outbox"));
    QVERIFY(QFile::exists(rootPath));
    QVERIFY(QFile::exists(outboxPath));

    // Kill the default resource.
    const QString oldResourceId = static_cast<SpecialCollections *>(SpecialMailCollections::self())->d->defaultResource().identifier();
    const AgentInstance resource = AgentManager::self()->instance(oldResourceId);
    QVERIFY(resource.isValid());
    AgentManager::self()->removeInstance(resource);
    QTest::qWait(100);

    // Run the DefaultResourceJob now. It should recover the Root and Outbox folders
    // created in testDefaultResourceJob.
    {
        DefaultResourceJob *resjob = new DefaultResourceJob(Settings::self(), this);
        resjob->setDefaultResourceType(QLatin1String("akonadi_maildir_resource"));

        QVariantMap options;
        options.insert(QLatin1String("Name"), i18nc("local mail folder", "Local Folders"));
        options.insert(QLatin1String("Path"), QString(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));
        resjob->setDefaultResourceOptions(options);
        resjob->setTypes(mDisplayNameMap.keys());
        resjob->setNameForTypeMap(mDisplayNameMap);
        resjob->setIconForTypeMap(mIconNameMap);

        AKVERIFYEXEC(resjob);
        QVERIFY(resjob->resourceId() != oldResourceId);   // Created another resource.
        Collection::List folders = resjob->specialCollections();
        QCOMPARE(folders.count(), 2);

        // Reorder the folders.
        if (folders.first().parentCollection() != Collection::root()) {
            folders.swap(0, 1);
        }

        // The first folder should be the Root.
        {
            Collection col = folders[0];
            QCOMPARE(col.name(), QLatin1String("Local Folders"));
            QVERIFY(col.hasAttribute<SpecialCollectionAttribute>());
            QCOMPARE(col.attribute<SpecialCollectionAttribute>()->collectionType(), QByteArray("local-mail"));
        }

        // The second folder should be the Outbox.
        {
            Collection col = folders[1];
            QCOMPARE(col.name(), QLatin1String("outbox"));
            QVERIFY(col.hasAttribute<SpecialCollectionAttribute>());
            QCOMPARE(col.attribute<SpecialCollectionAttribute>()->collectionType(), QByteArray("outbox"));
        }
    }
}

QTEST_AKONADIMAIN(LocalFoldersTest, NoGUI)
