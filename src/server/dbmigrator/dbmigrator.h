/*
    SPDX-FileCopyrightText: 2024 g10 Code GmbH

    SPDX-FileContributor: Daniel Vrátil <dvratil@kde.org>
*/

#pragma once

#include <QObject>

#include <functional>
#include <memory>
#include <stop_token>
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
    void stopMigration();

Q_SIGNALS:
    void info(const QString &message);
    void error(const QString &message);
    void progress(const QString &message, int tablesDone, int tablesTotal);
    void tableProgress(const QString &table, int rowsDone, int rowsTotal);

    void migrationCompleted(bool success);

private:
    bool runMigrationThread();
    bool copyTable(DataStore *sourceStore, DataStore *destStore, const TableDescription &table);

    bool migrateTables(DataStore *sourceStore, DataStore *destStore, DbConfig *destConfig);
    bool moveDatabaseToMainLocation(DbConfig *destConfig, const QString &destServerCfgFile);
    std::optional<QString> moveDatabaseToBackupLocation(DbConfig *config);
    std::optional<QString> backupAkonadiServerRc();

    void emitInfo(const QString &message);
    void emitError(const QString &message);
    void emitProgress(const QString &message, int tablesDone, int tablesTotal);
    void emitTableProgress(const QString &table, int done, int total);
    void emitCompletion(bool success);
    UIDelegate::Result questionYesNo(const QString &question);
    UIDelegate::Result questionYesNoSkip(const QString &question);

    QString m_targetEngine;
    std::unique_ptr<QThread> m_thread;
    std::stop_source m_stop;
    UIDelegate *m_uiDelegate = nullptr;
};

} // namespace Akonadi::Server
