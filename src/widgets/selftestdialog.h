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
/*!
 * \internal
 *
 * \brief A dialog that checks the current status of the Akonadi system.
 *
 * This dialog checks the current status of the Akonadi system and
 * displays a summary of the checks.
 *
 * \author Volker Krause <vkrause@kde.org>
 */
class AKONADIWIDGETS_EXPORT SelfTestDialog : public QDialog
{
    Q_OBJECT
public:
    /*!
     * Creates a new self test dialog.
     *
     * \a parent The parent widget.
     */
    explicit SelfTestDialog(QWidget *parent = nullptr);

    /*!
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
    AKONADIWIDGETS_NO_EXPORT QStandardItem *report(ResultType type, const KLocalizedString &summary, const KLocalizedString &details);
    AKONADIWIDGETS_NO_EXPORT QVariant serverSetting(const QString &group, const char *key, const QVariant &def) const;
    AKONADIWIDGETS_NO_EXPORT bool useStandaloneMysqlServer() const;
    AKONADIWIDGETS_NO_EXPORT bool runProcess(const QString &app, const QStringList &args, QString &result) const;

    AKONADIWIDGETS_NO_EXPORT void testSQLDriver();
    AKONADIWIDGETS_NO_EXPORT void testMySQLServer();
    AKONADIWIDGETS_NO_EXPORT void testMySQLServerLog();
    AKONADIWIDGETS_NO_EXPORT void testMySQLServerConfig();
    AKONADIWIDGETS_NO_EXPORT void testPSQLServer();
    AKONADIWIDGETS_NO_EXPORT void testAkonadiCtl();
    AKONADIWIDGETS_NO_EXPORT void testServerStatus();
    AKONADIWIDGETS_NO_EXPORT void testProtocolVersion();
    AKONADIWIDGETS_NO_EXPORT void testResources();
    AKONADIWIDGETS_NO_EXPORT void testServerLog();
    AKONADIWIDGETS_NO_EXPORT void testControlLog();
    AKONADIWIDGETS_NO_EXPORT void testRootUser();

    AKONADIWIDGETS_NO_EXPORT QString createReport();

    Ui::SelfTestDialog ui;
    QStandardItemModel *mTestModel = nullptr;
};
}
