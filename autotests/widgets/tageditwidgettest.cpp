/*
 * SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#include "qtest_akonadi.h"
#include <shared/aktest.h>

#include "monitor.h"
#include "tag.h"
#include "tagcreatejob.h"
#include "tagdeletejob.h"
#include "tageditwidget.h"
#include "tagmodel.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

#include <memory>
using namespace std::chrono_literals;
using namespace Akonadi;

/***
 * This test also covers TagManagementDialog and TagSelectionDialog, which
 * both wrap TagEditWidget and provide their own own Monitor and TagModel,
 * just one allows selection and the other does not
 */
class TagEditWidgetTest : public QObject
{
    Q_OBJECT

    struct TestSetup {
        TestSetup()
        {
            monitor = std::make_unique<Monitor>();
            monitor->setTypeMonitored(Monitor::Tags);

            model = std::make_unique<TagModel>(monitor.get());
            QSignalSpy modelSpy(model.get(), &TagModel::populated);
            QVERIFY(modelSpy.wait());
            QCOMPARE(model->rowCount(), 1); // there's one existing tag

            widget = std::make_unique<TagEditWidget>();
            widget->setModel(model.get());
            widget->show();
            QVERIFY(QTest::qWaitForWindowActive(widget.get()));

            newTagEdit = widget->findChild<QLineEdit *>(QStringLiteral("newTagEdit"));
            QVERIFY(newTagEdit);
            newTagButton = widget->findChild<QPushButton *>(QStringLiteral("newTagButton"));
            QVERIFY(newTagButton);
            QVERIFY(!newTagButton->isEnabled());
            tagsView = widget->findChild<QListView *>(QStringLiteral("tagsView"));
            QVERIFY(tagsView);
            tagDeleteButton = widget->findChild<QPushButton *>(QStringLiteral("tagDeleteButton"));
            QVERIFY(tagDeleteButton);
            QVERIFY(!tagDeleteButton->isVisible());

            valid = true;
        }

        ~TestSetup()
        {
            if (!createdTags.empty()) {
                auto *deleteJob = new TagDeleteJob(createdTags);
                AKVERIFYEXEC(deleteJob);
            }
        }

        bool createTags(int count)
        {
            const auto doCreateTags = [this, count]() {
                QSignalSpy monitorSpy(monitor.get(), &Monitor::tagAdded);
                for (int i = 0; i < count; ++i) {
                    auto *job = new TagCreateJob(Tag(QStringLiteral("TestTag-%1").arg(i)));
                    AKVERIFYEXEC(job);
                    createdTags.push_back(job->tag());
                }
                QTRY_COMPARE(monitorSpy.count(), count);
            };
            doCreateTags();
            return createdTags.size() == count;
        }

        bool checkSelectionIsEmpty() const
        {
            auto *const tagViewModel = tagsView->model();
            for (int i = 0; i < tagViewModel->rowCount(); ++i) {
                if (tagViewModel->data(tagViewModel->index(i, 0), Qt::CheckStateRole).value<Qt::CheckState>() != Qt::Unchecked) {
                    return false;
                }
            }
            return true;
        }

        QModelIndex indexForTag(const Tag &tag) const
        {
            for (int i = 0; i < tagsView->model()->rowCount(); ++i) {
                const auto index = tagsView->model()->index(i, 0);
                if (tagsView->model()->data(index, TagModel::TagRole).value<Tag>() == tag) {
                    return index;
                }
            }
            return {};
        }

        bool deleteTag(const Tag &tag, bool confirmDeletion)
        {
            const auto index = indexForTag(tag);
            AKVERIFY(index.isValid());
            const auto itemRect = tagsView->visualRect(index);
            // Hover over the item and confirm the button is there
            QTest::mouseMove(tagsView->viewport(), itemRect.center());
            AKVERIFY(QTest::qWaitFor(std::bind(&QWidget::isVisible, tagDeleteButton)));
            AKVERIFY(tagDeleteButton->geometry().intersects(itemRect));

            // Clicking the button blocks (QDialog::exec), so we need to confirm the
            // dialog from event loop
            bool confirmed = false;
            QTimer::singleShot(100ms, [this, confirmDeletion, &confirmed]() {
                confirmed = confirmDialog(confirmDeletion);
                QVERIFY(confirmed);
            });
            QTest::mouseClick(tagDeleteButton, Qt::LeftButton);

            // Check that the confirmation was successful
            AKVERIFY(confirmed);

            return true;
        }

        bool confirmDialog(bool confirmDeletion)
        {
            const auto windows = QApplication::topLevelWidgets();
            for (const auto *window : windows) {
                // We are using KMessageBox, which is not a QMessageBox but rather a custom QDialog
                if (window->objectName() == QLatin1String("questionYesNo")) {
                    const auto *const msgbox = qobject_cast<const QDialog *>(window);
                    AKVERIFY(msgbox);

                    const auto *const buttonBox = msgbox->findChild<const QDialogButtonBox *>();
                    AKVERIFY(buttonBox);
                    auto *const button = buttonBox->button(confirmDeletion ? QDialogButtonBox::Yes : QDialogButtonBox::No);
                    AKVERIFY(button);
                    QTest::mouseClick(button, Qt::LeftButton);
                    return true;
                }
            }

            return false;
        }

        std::unique_ptr<Monitor> monitor;
        std::unique_ptr<TagModel> model;
        std::unique_ptr<TagEditWidget> widget;

        QLineEdit *newTagEdit = nullptr;
        QPushButton *newTagButton = nullptr;
        QListView *tagsView = nullptr;
        QPushButton *tagDeleteButton = nullptr;

        Tag::List createdTags;

        bool valid = false;
    };

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testTagCreationWithEnter()
    {
        const auto tagName = QStringLiteral("TagEditWidgetTestTag");

        TestSetup test;
        QVERIFY(test.valid);

        QSignalSpy monitorSpy(test.monitor.get(), &Monitor::tagAdded);

        QTest::keyClicks(test.newTagEdit, tagName);
        QVERIFY(test.newTagButton->isEnabled());
        QTest::keyClick(test.newTagEdit, Qt::Key_Return);
        QVERIFY(!test.newTagButton->isEnabled());
        QVERIFY(!test.newTagEdit->isEnabled());

        QTRY_COMPARE(monitorSpy.size(), 1);
        test.createdTags.push_back(monitorSpy.at(0).at(0).value<Akonadi::Tag>());
        QCOMPARE(test.model->rowCount(), 2);
        QCOMPARE(test.model->data(test.model->index(1, 0), Qt::DisplayRole).toString(), tagName);
    }

    void testTagCreationWithButton()
    {
        const auto tagName = QStringLiteral("TagEditWidgetTestTag");

        TestSetup test;
        QVERIFY(test.valid);

        QSignalSpy monitorSpy(test.monitor.get(), &Monitor::tagAdded);

        QTest::keyClicks(test.newTagEdit, tagName);
        QVERIFY(test.newTagButton->isEnabled());
        QTest::mouseClick(test.newTagButton, Qt::LeftButton);
        QVERIFY(!test.newTagButton->isEnabled());
        QVERIFY(!test.newTagEdit->isEnabled());

        QTRY_COMPARE(monitorSpy.size(), 1);
        test.createdTags.push_back(monitorSpy.at(0).at(0).value<Akonadi::Tag>());
        QCOMPARE(test.model->rowCount(), 2);
        QCOMPARE(test.model->data(test.model->index(1, 0), Qt::DisplayRole).toString(), tagName);
    }

    void testDuplicatedTagCannotBeCreated()
    {
        TestSetup test;
        QVERIFY(test.valid);

        // Create a tag
        QVERIFY(test.createTags(1));

        // Wait for the tag to appear in the model
        QTRY_COMPARE(test.model->rowCount(), 2);

        // Type the entire string char-by-char - once the name is full the button
        // should be disabled because we don't allow creating duplicated tags
        const auto tagName = test.createdTags.front().name();
        for (int i = 0; i < tagName.size(); ++i) {
            QTest::keyClicks(test.newTagEdit, tagName[i]);
            QCOMPARE(test.newTagButton->isEnabled(), i != tagName.size() - 1);
        }
    }

    void testSettingSelectionFromCode()
    {
        TestSetup test;
        QVERIFY(test.valid);
        QVERIFY(test.createTags(10));

        test.widget->setSelectionEnabled(true);

        // Nothing should be checked
        QVERIFY(test.checkSelectionIsEmpty());

        // Set selection
        auto *model = test.tagsView->model();
        Tag::List selectTags;
        for (int i = 0; i < model->rowCount(); i += 2) {
            selectTags.push_back(model->data(model->index(i, 0), TagModel::TagRole).value<Tag>());
        }
        QVERIFY(!selectTags.empty());
        test.widget->setSelection(selectTags);
        QCOMPARE(test.widget->selection(), selectTags);

        // Confirm that the items are visually selected
        for (int i = 0; i < model->rowCount(); ++i) {
            const auto tag = model->data(model->index(i, 0), TagModel::TagRole).value<Tag>();
            const auto expectedState = selectTags.contains(tag) ? Qt::Checked : Qt::Unchecked;
            QCOMPARE(model->data(model->index(i, 0), Qt::CheckStateRole).value<Qt::CheckState>(), expectedState);
        }
    }

    void testSelectingTagsByMouse()
    {
        TestSetup test;
        QVERIFY(test.valid);
        QVERIFY(test.createTags(10));

        test.widget->setSelectionEnabled(true);

        // Nothing should be checked
        QVERIFY(test.checkSelectionIsEmpty());

        // Check several tags
        Tag::List selectedTags;
        auto *model = test.tagsView->model();
        for (int i = 0; i < model->rowCount(); i += 2) {
            const auto index = model->index(i, 0);
            selectedTags.push_back(model->data(index, TagModel::TagRole).value<Tag>());
            // Select the row
            QTest::mouseClick(test.tagsView->viewport(), Qt::LeftButton, {}, test.tagsView->visualRect(index).topLeft() + QPoint(5, 5));
            // Use spacebar to toggle selection, we can't possibly hit the checkbox with mouse in a
            // reliable manner.
            QTest::keyClick(test.tagsView, Qt::Key_Space);
        }

        // Confirm that the selection occurred
        for (int i = 0; i < model->rowCount(); ++i) {
            const auto expectedState = i % 2 == 0 ? Qt::Checked : Qt::Unchecked;
            QCOMPARE(model->data(model->index(i, 0), Qt::CheckStateRole).value<Qt::CheckState>(), expectedState);
        }

        // Compare the selectede tags
        auto currentSelection = test.widget->selection();
        const auto sortTag = [](const Tag &l, const Tag &r) {
            return l.id() < r.id();
        };
        std::sort(currentSelection.begin(), currentSelection.end(), sortTag);
        std::sort(selectedTags.begin(), selectedTags.end(), sortTag);
        QCOMPARE(currentSelection, selectedTags);
    }

    void testDeletingTags()
    {
        TestSetup test;
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        while (!test.createdTags.empty()) {
            QSignalSpy monitorSpy(test.monitor.get(), &Monitor::tagRemoved);
            // Get the last tag in the list and delete it
            const auto tag = test.createdTags.last();
            QVERIFY(test.deleteTag(tag, true));

            // Wait for confirmation
            QTRY_COMPARE(monitorSpy.size(), 1);
            QCOMPARE(monitorSpy.at(0).at(0).value<Tag>(), tag);

            test.createdTags.pop_back(); // remove the tag from the list
        }

        // Verify that we've deleted everything
        QVERIFY(test.createdTags.empty());
        QCOMPARE(test.model->rowCount(), 1); // only the default tag remains
    }

    void testRejectingDeleteDialogDoesntDeleteTheTAg()
    {
        TestSetup test;
        QVERIFY(test.valid);

        QSignalSpy monitorSpy(test.monitor.get(), &Monitor::tagRemoved);
        const auto index = test.model->index(0, 0);
        QVERIFY(index.isValid());

        const auto tag = test.model->data(index, TagModel::TagRole).value<Tag>();
        QVERIFY(test.deleteTag(tag, false));

        QTest::qWait(500); // wait some amount of time to see that nothing has changed...
        QVERIFY(monitorSpy.empty());
        QCOMPARE(test.model->rowCount(), 1);
    }
};

QTEST_AKONADIMAIN(TagEditWidgetTest)

#include "tageditwidgettest.moc"
