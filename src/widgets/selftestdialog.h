/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include "akonadiwidgets_export.h"
#include "ui_selftestdialog.h"
#include <QDialog>

class QStandardItem;
class QStandardItemModel;
namespace Akonadi
{
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
class AKONADIWIDGETS_EXPORT SelfTestDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * Creates a new self test dialog.
     *
     * @param parent The parent widget.
     */
    explicit SelfTestDialog(QWidget *parent = nullptr);

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
        Error,
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
    QStandardItemModel *mTestModel = nullptr;
};
}
