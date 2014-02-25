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

#include "localfoldersrequestjobtest.h"

#include "../../collectionpathresolver_p.h"

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
#include <akonadi/qtest_akonadi.h>
#include "../../specialcollectionattribute_p.h"
#include "../../specialcollections_p.h"
#include <akonadi/kmime/specialmailcollections.h>
#include <akonadi/kmime/specialmailcollectionsrequestjob.h>
#include "../specialmailcollectionstesting_p.h"
#include "../../specialcollectionshelperjobs_p.h"

using namespace Akonadi;

void LocalFoldersRequestJobTest::initTestCase()
{
    qRegisterMetaType<Akonadi::AgentInstance>();

    QVERIFY(Control::start());
    QTest::qWait(1000);

    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    Q_UNUSED(smc);

    // No one has created the default resource.  LF has no folders.
    QCOMPARE(smct->_t_knownResourceCount(), 0);
    QCOMPARE(smct->_t_knownFolderCount(), 0);
}

void LocalFoldersRequestJobTest::testRequestWithNoDefaultResourceExisting()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    QSignalSpy spy(smc, SIGNAL(collectionsChanged(Akonadi::AgentInstance)));
    QSignalSpy defSpy(smc, SIGNAL(defaultCollectionsChanged()));
    QVERIFY(spy.isValid());
    QVERIFY(defSpy.isValid());

    // Initially the defaut maildir does not exist.
    QVERIFY(!QFile::exists(KGlobal::dirs()->localxdgdatadir() + "local-mail"));

    // Request some default folders.
    {
        SpecialMailCollectionsRequestJob *rjob = new SpecialMailCollectionsRequestJob(this);
        rjob->requestDefaultCollection(SpecialMailCollections::Outbox);
        rjob->requestDefaultCollection(SpecialMailCollections::Drafts);
        AKVERIFYEXEC(rjob);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(defSpy.count(), 1);
        QCOMPARE(smct->_t_knownResourceCount(), 1);
        QCOMPARE(smct->_t_knownFolderCount(), 3);   // Outbox, Drafts, and Root.
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Drafts));
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Root));
    }

    // The maildir should exist now.
    QVERIFY(QFile::exists(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));
}

void LocalFoldersRequestJobTest::testRequestWithDefaultResourceAlreadyExisting()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    QSignalSpy spy(smc, SIGNAL(collectionsChanged(Akonadi::AgentInstance)));
    QSignalSpy defSpy(smc, SIGNAL(defaultCollectionsChanged()));
    QVERIFY(spy.isValid());
    QVERIFY(defSpy.isValid());

    // Prerequisites (from testRequestWithNoDefaultResourceExisting()).
    QVERIFY(QFile::exists(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));
    QVERIFY(!smc->hasDefaultCollection(SpecialMailCollections::Inbox));
    QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
    const Collection oldOutbox = smc->defaultCollection(SpecialMailCollections::Outbox);

    // Request some default folders.
    {
        SpecialMailCollectionsRequestJob *rjob = new SpecialMailCollectionsRequestJob(this);
        rjob->requestDefaultCollection(SpecialMailCollections::Outbox);   // Exists previously.
        rjob->requestDefaultCollection(SpecialMailCollections::Inbox);   // Must be created.
        AKVERIFYEXEC(rjob);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(defSpy.count(), 1);
        QCOMPARE(smct->_t_knownResourceCount(), 1);
        QCOMPARE(smct->_t_knownFolderCount(), 4);   // Inbox, Outbox, Drafts, and Root.
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Inbox));
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Drafts));
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Root));
    }

    // This should be untouched.
    QCOMPARE(smc->defaultCollection(SpecialMailCollections::Outbox), oldOutbox);
}

void LocalFoldersRequestJobTest::testMixedRequest()
{
    SpecialMailCollections *smc = SpecialMailCollections::self();
    SpecialMailCollectionsTesting *smct = SpecialMailCollectionsTesting::_t_self();
    Q_ASSERT(smc);
    Q_ASSERT(smct);
    QSignalSpy spy(smc, SIGNAL(collectionsChanged(Akonadi::AgentInstance)));
    QSignalSpy defSpy(smc, SIGNAL(defaultCollectionsChanged()));
    QVERIFY(spy.isValid());
    QVERIFY(defSpy.isValid());

    // Get our knut collection.
    Collection res1;
    {
        CollectionPathResolver *resolver = new CollectionPathResolver("res1", this);
        QVERIFY(resolver->exec());
        res1 = Collection(resolver->collection());
        CollectionFetchJob *fjob = new CollectionFetchJob(res1, CollectionFetchJob::Base, this);
        AKVERIFYEXEC(fjob);
        Q_ASSERT(fjob->collections().count() == 1);
        res1 = fjob->collections().first();
        QVERIFY(res1.isValid());
        QVERIFY(!res1.resource().isEmpty());
    }

    // Create a LocalFolder in the knut resource.
    Collection knutOutbox;
    {
        knutOutbox.setName(QLatin1String("my_outbox"));
        knutOutbox.setParentCollection(res1);
        kDebug() << res1;
        knutOutbox.addAttribute(new SpecialCollectionAttribute("outbox"));
        CollectionCreateJob *cjob = new CollectionCreateJob(knutOutbox, this);
        AKVERIFYEXEC(cjob);
        knutOutbox = cjob->collection();
    }

    // Prerequisites (from the above two functions).
    QVERIFY(QFile::exists(KGlobal::dirs()->localxdgdatadir() + QLatin1String("local-mail")));
    QVERIFY(!smc->hasDefaultCollection(SpecialMailCollections::SentMail));
    QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
    const Collection oldOutbox = smc->defaultCollection(SpecialMailCollections::Outbox);

    // Request some folders, both in our default resource and in the knut resource.
    {
        SpecialMailCollectionsRequestJob *rjob = new SpecialMailCollectionsRequestJob(this);
        rjob->requestDefaultCollection(SpecialMailCollections::Outbox);   // Exists previously.
        rjob->requestDefaultCollection(SpecialMailCollections::SentMail);   // Must be created.
        rjob->requestCollection(SpecialMailCollections::Outbox, AgentManager::self()->instance(res1.resource()));     // Exists previously, but unregistered with LF.
        rjob->requestCollection(SpecialMailCollections::SentMail, AgentManager::self()->instance(res1.resource()));     // Must be created.
        AKVERIFYEXEC(rjob);
        QCOMPARE(spy.count(), 2);   // Default resource and knut resource.
        QCOMPARE(defSpy.count(), 1);
        QCOMPARE(smct->_t_knownResourceCount(), 2);
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::Outbox));
        QVERIFY(smc->hasDefaultCollection(SpecialMailCollections::SentMail));
        QVERIFY(smc->hasCollection(SpecialMailCollections::Outbox, AgentManager::self()->instance(res1.resource())));
        QVERIFY(smc->hasCollection(SpecialMailCollections::SentMail, AgentManager::self()->instance(res1.resource())));
    }

    // These should be untouched.
    QCOMPARE(smc->defaultCollection(SpecialMailCollections::Outbox), oldOutbox);
    QCOMPARE(smc->collection(SpecialMailCollections::Outbox, AgentManager::self()->instance(res1.resource())), knutOutbox);
}

QTEST_AKONADIMAIN(LocalFoldersRequestJobTest, NoGUI)
