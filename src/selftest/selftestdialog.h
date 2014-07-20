/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#ifndef AKONADI_SELFTESTDIALOG_P_H
#define AKONADI_SELFTESTDIALOG_P_H

#include "ui_selftestdialog.h"

#include <QDialog>
#include <KLocalizedString>

class QStandardItem;
class QStandardItemModel;

/**
 * @internal
 *
 * @short A dialog that checks the current status of the Akonadi system.
 *
 * This dialog checks the current status of the Akonadi system and
 * displays a summary of the checks.
 *
 * @author Volker Krause <vkrause@kde.org>
 */
class SelfTestDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * Creates a new self test dialog.
     *
     * @param parent The parent widget.
     */
    explicit SelfTestDialog(QWidget *parent = 0);

    /**
     * Hides the label with the introduction message.
     */
    void hideIntroduction();

private Q_SLOTS:
    void selectionChanged(const QModelIndex &index);
    void saveReport();
    void copyReport();
    void linkActivated(const QString &link);
    void runTests();

private:
    enum ResultType {
        Skip,
        Success,
        Warning,
        Error
    };
    QStandardItem *report(ResultType type, const KLocalizedString &summary, const KLocalizedString &details);
    QVariant serverSetting(const QString &group, const char *key, const QVariant &def) const;
    bool useStandaloneMysqlServer() const;
    bool runProcess(const QString &app, const QStringList &args, QString &result) const;

    void testSQLDriver();
    void testMySQLServer();
    void testMySQLServerLog();
    void testMySQLServerConfig();
    void testPSQLServer();
    void testAkonadiCtl();
    void testServerStatus();
    void testProtocolVersion();
    void testResources();
    void testServerLog();
    void testControlLog();
    void testRootUser();

    QString createReport();

    Ui::SelfTestDialog ui;
    QStandardItemModel *mTestModel;
};

#endif
