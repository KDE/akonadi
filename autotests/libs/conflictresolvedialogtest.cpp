/*
    SPDX-FileCopyrightText: 2017-2022 Laurent Montel <montel@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "conflictresolvedialogtest.h"
#include "../src/widgets/conflictresolvedialog_p.h"

#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QTextBrowser>

QTEST_MAIN(ConflictResolveDialogTest)

ConflictResolveDialogTest::ConflictResolveDialogTest(QObject *parent)
    : QObject(parent)
{
}

void ConflictResolveDialogTest::shouldHaveDefaultValues()
{
    Akonadi::ConflictResolveDialog dlg;

    QVERIFY(!dlg.windowTitle().isEmpty());

    auto takeLeftButton = dlg.findChild<QPushButton *>(QStringLiteral("takeLeftButton"));
    QVERIFY(takeLeftButton);
    QVERIFY(!takeLeftButton->text().isEmpty());

    auto takeRightButton = dlg.findChild<QPushButton *>(QStringLiteral("takeRightButton"));
    QVERIFY(takeRightButton);
    QVERIFY(!takeRightButton->text().isEmpty());

    auto keepBothButton = dlg.findChild<QPushButton *>(QStringLiteral("keepBothButton"));
    QVERIFY(keepBothButton);
    QVERIFY(!keepBothButton->text().isEmpty());
    QVERIFY(keepBothButton->isDefault());

    auto mView = dlg.findChild<QTextBrowser *>(QStringLiteral("view"));
    QVERIFY(mView);
    QVERIFY(mView->toPlainText().isEmpty());

    auto docuLabel = dlg.findChild<QLabel *>(QStringLiteral("doculabel"));
    QVERIFY(docuLabel);
    QVERIFY(!docuLabel->text().isEmpty());
    QVERIFY(docuLabel->wordWrap());
    QCOMPARE(docuLabel->contextMenuPolicy(), Qt::NoContextMenu);
}
