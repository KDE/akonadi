/*
 * Copyright 2020  Daniel Vrátil <dvratil@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "qtest_akonadi.h"
#include <qnamespace.h>
#include <qtestmouse.h>
#include <shared/aktest.h>
#include <shared/akscopeguard.h>
#include "../libs/test_utils.h"

#include "subscriptiondialog.h"
#include "subscriptionmodel_p.h"
#include "subscriptionjob_p.h"

#include <QSignalSpy>
#include <QTest>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeView>
#include <QDialogButtonBox>
#include <QDialog>
#include <QCheckBox>

#include <memory>
#include <deque>

using namespace Akonadi;

class SubscriptionDialogTest: public QObject
{
    Q_OBJECT

    struct TestSetup {
        enum { defaultCollectionCount = 7 };


        TestSetup()
        {
            widget = std::make_unique<SubscriptionDialog>(QStringList{
                    Collection::mimeType(),
                    QStringLiteral("application/octet-stream")
            });
            widget->setAttribute(Qt::WA_DeleteOnClose, false);
            widget->show();

            model = widget->findChild<SubscriptionModel*>();
            QVERIFY(model);
            QSignalSpy modelLoadedSpy(model, &SubscriptionModel::loaded);

            buttonBox = widget->findChild<QDialogButtonBox*>();
            QVERIFY(buttonBox);
            QVERIFY(!buttonBox->button(QDialogButtonBox::Ok)->isEnabled());
            searchLineEdit = widget->findChild<QLineEdit*>(QStringLiteral("searchLineEdit"));
            QVERIFY(searchLineEdit);
            subscribedOnlyChkBox = widget->findChild<QCheckBox*>(QStringLiteral("subscribedOnlyCheckBox"));
            QVERIFY(subscribedOnlyChkBox);
            collectionView = widget->findChild<QTreeView*>(QStringLiteral("collectionView"));
            QVERIFY(collectionView);
            subscribeButton = widget->findChild<QPushButton*>(QStringLiteral("subscribeButton"));
            QVERIFY(subscribeButton);
            unsubscribeButton = widget->findChild<QPushButton*>(QStringLiteral("unsubscribeButton"));
            QVERIFY(unsubscribeButton);

            QVERIFY(QTest::qWaitForWindowActive(widget.get()));
            QVERIFY(modelLoadedSpy.wait());
            QTest::qWait(100);

            // Helps with testing :)
            collectionView->expandAll();

            // Post-setup conditions
            QCOMPARE(countTotalRows(), defaultCollectionCount);
            QVERIFY(buttonBox->button(QDialogButtonBox::Ok)->isEnabled());

            valid = true;
        }

        ~TestSetup()
        {
        }

        int countTotalRows(const QModelIndex &parent = {}) const
        {
            const auto count = collectionView->model()->rowCount(parent);
            int total = count;
            for (int i = 0; i < count; ++i) {
                total += countTotalRows(collectionView->model()->index(i, 0, parent));
            }
            return total;
        }

        static bool unsubscribeCollection(const Collection &col)
        {
            return modifySubscription({}, {col});
        }

        static bool subscribeCollection(const Collection &col)
        {
            return modifySubscription({col}, {});
        }

        static bool modifySubscription(const Collection::List &subscribe,
                                       const Collection::List &unsubscribe)
        {
            auto job = new SubscriptionJob();
            job->subscribe(subscribe);
            job->unsubscribe(unsubscribe);
            bool ok = false;
            [job, &ok]() { AKVERIFYEXEC(job); ok = true; }();
            AKVERIFY(ok);

            return true;
        }

        bool selectCollection(const Collection &col)
        {
            AKVERIFY(col.isValid());
            const QModelIndex colIdx = indexForCollection(col);
            AKVERIFY(colIdx.isValid());

            collectionView->scrollTo(colIdx);
            QTest::mouseClick(collectionView->viewport(), Qt::LeftButton, {}, collectionView->visualRect(colIdx).center());

            AKCOMPARE(collectionView->currentIndex(), colIdx);
            return true;
        }

        bool isCollectionChecked(const Collection &col) const
        {
            AKVERIFY(col.isValid());
            const auto colIdx = indexForCollection(col);
            AKVERIFY(colIdx.isValid());

            return collectionView->model()->data(colIdx, Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked;
        }

        void acceptDialog()
        {
            auto button = buttonBox->button(QDialogButtonBox::Ok);
            QTest::mouseClick(button, Qt::LeftButton);
        }

        QModelIndex indexForCollection(const Collection &col) const
        {
            auto model = collectionView->model();
            std::deque<QModelIndex> idxQueue;
            idxQueue.push_back(QModelIndex{});
            while (!idxQueue.empty()) {
                const auto idx = idxQueue.front();
                idxQueue.pop_front();
                if (model->data(idx, CollectionModel::CollectionIdRole).value<qint64>() == col.id()) {
                    return idx;
                }
                for (int i = 0; i < model->rowCount(idx); ++i) {
                    idxQueue.push_back(model->index(i, 0, idx));
                }
            }
            return {};
        }

        std::unique_ptr<SubscriptionDialog> widget;
        QDialogButtonBox *buttonBox = nullptr;
        QLineEdit *searchLineEdit = nullptr;
        QCheckBox *subscribedOnlyChkBox = nullptr;
        QTreeView *collectionView = nullptr;
        QPushButton *subscribeButton = nullptr;
        QPushButton *unsubscribeButton = nullptr;
        SubscriptionModel *model = nullptr;

        bool valid = false;
    };

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testSearchFilter()
    {
        TestSetup test;
        QVERIFY(test.valid);

        QTest::keyClicks(test.searchLineEdit, QStringLiteral("foo"));
        QCOMPARE(test.countTotalRows(), 2);
    }

    void testSubscribedOnlyCheckbox()
    {
        const auto col = Collection{collectionIdFromPath(QStringLiteral("res1/foo/bla"))};
        const AkScopeGuard guard([col]() {
            TestSetup::subscribeCollection(col);
        });

        QVERIFY(TestSetup::unsubscribeCollection(col));

        TestSetup test;
        QVERIFY(test.valid);

        test.subscribedOnlyChkBox->setChecked(true);

        QTRY_COMPARE(test.countTotalRows(), test.defaultCollectionCount - 1);

        test.subscribedOnlyChkBox->setChecked(false);

        QTRY_COMPARE(test.countTotalRows(), test.defaultCollectionCount);
    }

    void testSubscribeButton()
    {
        const auto col = Collection{collectionIdFromPath(QStringLiteral("res1/foo/bla"))};
        const AkScopeGuard guard([col]() {
            TestSetup::subscribeCollection(col);
        });

        QVERIFY(TestSetup::unsubscribeCollection(col));

        TestSetup test;
        QVERIFY(test.valid);

        QVERIFY(test.selectCollection(col));
        QTest::mouseClick(test.subscribeButton, Qt::LeftButton);
        QVERIFY(test.isCollectionChecked(col));

        auto monitor = getTestMonitor();
        QSignalSpy monitorSpy(monitor.get(), &Monitor::collectionSubscribed);
        test.acceptDialog();
        QVERIFY(monitorSpy.wait());
    }

    void testUnsubscribeButton()
    {
        const auto col = Collection{collectionIdFromPath(QStringLiteral("res1/foo/bla"))};

        TestSetup test;
        QVERIFY(test.valid);

        QVERIFY(test.selectCollection(col));
        QTest::mouseClick(test.unsubscribeButton, Qt::LeftButton);
        QVERIFY(!test.isCollectionChecked(col));

        auto monitor = getTestMonitor();
        QSignalSpy monitorSpy(monitor.get(), &Monitor::collectionUnsubscribed);
        test.acceptDialog();
        QVERIFY(monitorSpy.wait());
    }
};

QTEST_AKONADIMAIN(SubscriptionDialogTest)

#include "subscriptiondialogtest.moc"
