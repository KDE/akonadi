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
#include <iterator>
#include <qnamespace.h>
#include <qtestmouse.h>
#include <shared/aktest.h>

#include "tagwidget.h"
#include "tagselectiondialog.h"
#include "tagmodel.h"
#include "monitor.h"
#include "tag.h"
#include "tagdeletejob.h"
#include "tagcreatejob.h"

#include <QSignalSpy>
#include <QTest>
#include <QLineEdit>
#include <QToolButton>
#include <QDialogButtonBox>
#include <QPushButton>

#include <memory>

using namespace Akonadi;

class TagWidgetTest: public QObject
{
    Q_OBJECT

    struct TestSetup {
        TestSetup()
        {
            widget = std::make_unique<TagWidget>();
            widget->show();

            monitor = widget->findChild<Monitor*>();
            QVERIFY(monitor);
            model = widget->findChild<TagModel*>();
            QVERIFY(model);
            QSignalSpy modelSpy(model, &TagModel::populated);
            QVERIFY(modelSpy.wait());

            QVERIFY(QTest::qWaitForWindowActive(widget.get()));

            tagView = widget->findChild<QLineEdit*>(QStringLiteral("tagView"));
            QVERIFY(tagView);
            QVERIFY(tagView->isReadOnly()); // always read-only
            editButton = widget->findChild<QToolButton*>(QStringLiteral("editButton"));
            QVERIFY(editButton);

            valid = true;
        }

        ~TestSetup()
        {
            if (!createdTags.empty()) {
                auto deleteJob = new TagDeleteJob(createdTags);
                AKVERIFYEXEC(deleteJob);
            }
        }

        bool createTags(int count)
        {
            const auto doCreateTags = [this, count]() {
                QSignalSpy monitorSpy(monitor, &Monitor::tagAdded);
                for (int i = 0; i < count; ++i) {
                    auto job = new TagCreateJob(Tag(QStringLiteral("TestTag-%1").arg(i)));
                    AKVERIFYEXEC(job);
                    createdTags.push_back(job->tag());
                }
                QTRY_COMPARE(monitorSpy.count(), count);
            };
            doCreateTags();
            return createdTags.size() == count;
        }

        bool testSelectionMatches(QSignalSpy &selectionSpy, const Tag::List &selection)
        {
            QStringList names;
            std::transform(selection.begin(), selection.end(), std::back_inserter(names), std::bind(&Tag::name, std::placeholders::_1));

            AKCOMPARE(widget->selection(), selection);
            AKCOMPARE(selectionSpy.size(), 1);

            AKCOMPARE(selectionSpy.at(0).at(0).value<Tag::List>(), selection);
            AKCOMPARE(tagView->text(), names.join(QStringLiteral(", ")));
            return true;
        }

        bool selectTagsInDialog(const Tag::List &selection)
        {
            const auto windows = QApplication::topLevelWidgets();
            for (auto *window : windows) {
                if (auto *dlg = qobject_cast<TagSelectionDialog*>(window)) {
                    // Set the selection through code, testing selecting tags with mouse is
                    // out-of-scope for this test, there's a dedicated TagEditWidget test for that.
                    dlg->setSelection(selection);
                    auto *button = dlg->buttons()->button(QDialogButtonBox::Ok);
                    AKVERIFY(button);
                    QTest::mouseClick(button, Qt::LeftButton);
                    return true;
                }
            }

            return false;
        }

        std::unique_ptr<TagWidget> widget;
        Monitor *monitor = nullptr;
        TagModel *model = nullptr;

        QLineEdit *tagView = nullptr;
        QToolButton *editButton = nullptr;

        Tag::List createdTags;

        bool valid = false;
    };

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testInitialState()
    {
        TestSetup test;
        QVERIFY(test.valid);

        QVERIFY(test.tagView->text().isEmpty());
        QVERIFY(test.widget->selection().isEmpty());
    }

    void testSettingSelectionFromCode()
    {
        TestSetup test;
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        QSignalSpy selectionSpy(test.widget.get(), &TagWidget::selectionChanged);
        const auto selection = Tag::List{test.createdTags[1], test.createdTags[3]};
        test.widget->setSelection(selection);

        QVERIFY(test.testSelectionMatches(selectionSpy, selection));
    }

    void testSettingSelectionViaDialog()
    {
        TestSetup test;
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        QSignalSpy selectionSpy(test.widget.get(), &TagWidget::selectionChanged);
        const auto selection = Tag::List{test.createdTags[1], test.createdTags[3]};

        bool ok = false;
        // Clicking on the Edit button opens the dialog in a blocking way, so
        // we need to dispatch the test from event loop
        QTimer::singleShot(100, this, [&test, &selection, &ok]() { QVERIFY(test.selectTagsInDialog(selection)); ok = true; });
        QTest::mouseClick(test.editButton, Qt::LeftButton);
        QVERIFY(ok);

        QVERIFY(test.testSelectionMatches(selectionSpy, selection));
    }

    void testClearTagsFromCode()
    {
        TestSetup test;
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        const auto selection = Tag::List{test.createdTags[1], test.createdTags[3]};
        test.widget->setSelection(selection);
        QCOMPARE(test.widget->selection(), selection);

        QSignalSpy selectionSpy(test.widget.get(), &TagWidget::selectionChanged);
        test.widget->clearTags();
        QVERIFY(test.widget->selection().isEmpty());
        QCOMPARE(selectionSpy.size(), 1);
        QVERIFY(selectionSpy.at(0).at(0).value<Tag::List>().empty());
        QVERIFY(test.tagView->text().isEmpty());
    }
};

QTEST_AKONADIMAIN(TagWidgetTest)

#include "tagwidgettest.moc"


