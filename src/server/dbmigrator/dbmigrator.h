/*
    SPDX-FileCopyrightText: 2024 g10 Code GmbH
    SPDX-FileContributor: Daniel Vrátil <dvratil@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <QObject>

#include <functional>
#include <memory>
#include <system_error>
#include <variant>

class QThread;

namespace Akonadi::Server
{

class DbConfig;
class TableDescription;
class DataStore;

class UIDelegate
{
public:
    enum class Result {
        Yes,
        No,
        Skip,
    };

    virtual ~UIDelegate() = default;
    virtual Result questionYesNo(const QString &question) = 0;
    virtual Result questionYesNoSkip(const QString &question) = 0;
};

class DbMigrator : public QObject
{
    Q_OBJECT
public:
    explicit DbMigrator(const QString &targetEngine, UIDelegate *delegate, QObject *parent = nullptr);
    ~DbMigrator() override;

    void startMigration();

Q_SIGNALS:
    void info(const QString &message);
    void error(const QString &message);
    void progress(const QString &message, int tablesDone, int tablesTotal);
    void tableProgress(const QString &table, int rowsDone, int rowsTotal);

    void migrationCompleted(bool success);

private:
    [[nodiscard]] bool runMigrationThread();
    bool copyTable(DataStore *sourceStore, DataStore *destStore, const TableDescription &table);

    [[nodiscard]] bool migrateTables(DataStore *sourceStore, DataStore *destStore, DbConfig *destConfig);
    [[nodiscard]] bool moveDatabaseToMainLocation(DbConfig *destConfig, const QString &destServerCfgFile);
    std::optional<QString> moveDatabaseToBackupLocation(DbConfig *config);
    std::optional<QString> backupAkonadiServerRc();
    bool runStorageJanitor(DbConfig *sourceConfig);

    void emitInfo(const QString &message);
    void emitError(const QString &message);
    void emitProgress(const QString &message, int tablesDone, int tablesTotal);
    void emitTableProgress(const QString &table, int done, int total);
    void emitCompletion(bool success);
    [[nodiscard]] UIDelegate::Result questionYesNo(const QString &question);
    [[nodiscard]] UIDelegate::Result questionYesNoSkip(const QString &question);

    QString m_targetEngine;
    std::unique_ptr<QThread> m_thread;
    UIDelegate *m_uiDelegate = nullptr;
};

} // namespace Akonadi::Server
