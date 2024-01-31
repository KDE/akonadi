/*
 * SPDX-FileCopyrightText: 2020 Daniel Vrátil <dvratil@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 */

#include "qtest_akonadi.h"
#include "shared/aktest.h"

#include "monitor.h"
#include "tag.h"
#include "tagcreatejob.h"
#include "tagdeletejob.h"
#include "tagmodel.h"
#include "tagselectioncombobox.h"

#include <QAbstractItemView>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>

#include <memory>

using namespace Akonadi;

class TagSelectionComboBoxTest : public QObject
{
    Q_OBJECT

    struct TestSetup {
        explicit TestSetup(bool checkable)
            : widget(std::make_unique<TagSelectionComboBox>())
        {
            widget->setCheckable(checkable);
            widget->show();

            monitor = widget->findChild<Monitor *>();
            QVERIFY(monitor);
            model = widget->findChild<TagModel *>();
            QVERIFY(model);
            QSignalSpy modelSpy(model, &TagModel::populated);
            QVERIFY(modelSpy.wait());

            QVERIFY(QTest::qWaitForWindowActive(widget.get()));

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

        bool testSelectionMatches(QSignalSpy &selectionSpy, const Tag::List &selection) const
        {
            QStringList names;
            std::transform(selection.begin(), selection.end(), std::back_inserter(names), std::bind(&Tag::name, std::placeholders::_1));

            AKCOMPARE(widget->selection(), selection);
            AKCOMPARE(widget->selectionNames(), names);
            AKCOMPARE(selectionSpy.size(), 1);

            AKCOMPARE(selectionSpy.at(0).at(0).value<Tag::List>(), selection);
            AKCOMPARE(widget->currentText(), QLocale{}.createSeparatedList(names));
            return true;
        }

        bool selectTagsInComboBox(const Tag::List & /*selection*/)
        {
            const auto windows = QApplication::topLevelWidgets();
            for (auto window : windows) {
                if (auto combo = qobject_cast<TagSelectionComboBox *>(window)) {
                    QTest::mouseClick(combo, Qt::LeftButton);
                    return true;
                }
            }

            return false;
        }

        bool toggleDropdown() const
        {
            auto view = widget->view()->parentWidget();
            const bool visible = view->isVisible();
            QTest::mouseClick(widget->lineEdit(), Qt::LeftButton);
            QTest::qWait(10);
            AKCOMPARE(view->isVisible(), !visible);

            return true;
        }

        QModelIndex indexForTag(const Tag &tag) const
        {
            for (int i = 0; i < widget->model()->rowCount(); ++i) {
                const auto index = widget->model()->index(i, 0);
                if (widget->model()->data(index, TagModel::TagRole).value<Tag>().name() == tag.name()) {
                    return index;
                }
            }
            return {};
        }

        std::unique_ptr<TagSelectionComboBox> widget;
        Monitor *monitor = nullptr;
        TagModel *model = nullptr;

        Tag::List createdTags;

        bool valid = false;
    };

public:
    TagSelectionComboBoxTest()
    {
        qRegisterMetaType<Akonadi::Tag::List>();
    }

private Q_SLOTS:
    void initTestCase()
    {
        AkonadiTest::checkTestIsIsolated();
    }

    void testInitialState()
    {
        TestSetup test{true};
        QVERIFY(test.valid);

        QVERIFY(test.widget->currentText().isEmpty());
        QVERIFY(test.widget->selection().isEmpty());
    }

    void testSettingSelectionFromCode()
    {
        TestSetup test{true};
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        QSignalSpy selectionSpy(test.widget.get(), &TagSelectionComboBox::selectionChanged);
        const auto selection = Tag::List{test.createdTags[1], test.createdTags[3]};
        test.widget->setSelection(selection);

        QVERIFY(test.testSelectionMatches(selectionSpy, selection));
    }

    void testSettingSelectionByName()
    {
        TestSetup test{true};
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        QSignalSpy selectionSpy(test.widget.get(), &TagSelectionComboBox::selectionChanged);
        const auto selection = QStringList{test.createdTags[1].name(), test.createdTags[3].name()};
        test.widget->setSelection(selection);

        QVERIFY(test.testSelectionMatches(selectionSpy, {test.createdTags[1], test.createdTags[3]}));
    }

    void testSelectionByKeyboard()
    {
        TestSetup test{true};
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        QSignalSpy selectionSpy(test.widget.get(), &TagSelectionComboBox::selectionChanged);
        const auto selection = Tag::List{test.createdTags[1], test.createdTags[3]};

        QVERIFY(!test.widget->view()->parentWidget()->isVisible());
        QVERIFY(test.toggleDropdown());

        QTest::keyClick(test.widget->view(), Qt::Key_Down); // from name to tag 1
        QTest::keyClick(test.widget->view(), Qt::Key_Down); // from tag 1 to tag 2
        QTest::keyClick(test.widget->view(), Qt::Key_Space); // select tag 2
        QTest::keyClick(test.widget->view(), Qt::Key_Down); // from tag 2 to tag 3
        QTest::keyClick(test.widget->view(), Qt::Key_Down); // from tag 3 to tag 4
        QTest::keyClick(test.widget->view(), Qt::Key_Space); // select tag 4

        QTest::keyClick(test.widget->view(), Qt::Key_Escape); // close
        QTest::qWait(100);
        QVERIFY(!test.widget->view()->parentWidget()->isVisible());

        QCOMPARE(selectionSpy.size(), 2); // two selections -> two signals
        selectionSpy.takeFirst(); // remove the first one
        QVERIFY(test.testSelectionMatches(selectionSpy, selection));
    }

    void testNonCheckableSelection()
    {
        TestSetup test{false};
        QVERIFY(test.valid);
        QVERIFY(test.createTags(4));

        test.widget->setCurrentIndex(1);
        QCOMPARE(test.widget->currentData(TagModel::TagRole).value<Tag>(), test.createdTags[0]);

        QCOMPARE(test.widget->selection(), Tag::List{test.createdTags[0]});
        QCOMPARE(test.widget->selectionNames(), QStringList{test.createdTags[0].name()});

        test.widget->setSelection({test.createdTags[1]});
        QCOMPARE(test.widget->currentIndex(), 2);
    }
};

QTEST_AKONADIMAIN(TagSelectionComboBoxTest)

#include "tagselectioncomboboxtest.moc"
