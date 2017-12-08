/*
    Copyright (c) 2017 Laurent Montel <montel@kde.org>

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

    QPushButton *takeLeftButton = dlg.findChild<QPushButton *>(QStringLiteral("takeLeftButton"));
    QVERIFY(takeLeftButton);
    QVERIFY(!takeLeftButton->text().isEmpty());

    QPushButton *takeRightButton = dlg.findChild<QPushButton *>(QStringLiteral("takeRightButton"));
    QVERIFY(takeRightButton);
    QVERIFY(!takeRightButton->text().isEmpty());

    QPushButton *keepBothButton = dlg.findChild<QPushButton *>(QStringLiteral("keepBothButton"));
    QVERIFY(keepBothButton);
    QVERIFY(!keepBothButton->text().isEmpty());
    QVERIFY(keepBothButton->isDefault());

    QTextBrowser *mView = dlg.findChild<QTextBrowser *>(QStringLiteral("view"));
    QVERIFY(mView);
    QVERIFY(mView->toPlainText().isEmpty());

    QLabel *docuLabel = dlg.findChild<QLabel *>(QStringLiteral("doculabel"));
    QVERIFY(docuLabel);
    QVERIFY(!docuLabel->text().isEmpty());

    QPushButton *openEditorButton = dlg.findChild<QPushButton *>(QStringLiteral("openEditorButton"));
    QVERIFY(openEditorButton);
    QVERIFY(!openEditorButton->text().isEmpty());
}
