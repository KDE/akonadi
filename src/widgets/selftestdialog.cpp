/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "selftestdialog.h"
#include "agentmanager.h"
#include "private/protocol_p.h"
#include "private/standarddirs_p.h"
#include "servermanager.h"
#include "servermanager_p.h"

#include <KLocalizedString>
#include <KUser>
#include <QFileDialog>
#include <QIcon>
#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QStandardPaths>
#include <QUrl>

#include <QApplication>
#include <QClipboard>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDate>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardItemModel>
#include <QTextStream>
#include <QVBoxLayout>

/// @cond PRIVATE

using namespace Akonadi;

static QString makeLink(const QString &file)
{
    return QStringLiteral("<a href=\"%1\">%2</a>").arg(file, file);
}

enum SelfTestRole {
    ResultTypeRole = Qt::UserRole,
    FileIncludeRole,
    ListDirectoryRole,
    EnvVarRole,
    SummaryRole,
    DetailsRole,
};

SelfTestDialog::SelfTestDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window", "Akonadi Server Self-Test"));
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    auto mainWidget = new QWidget(this);
    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(mainWidget);
    auto user1Button = new QPushButton(this);
    buttonBox->addButton(user1Button, QDialogButtonBox::ActionRole);
    auto user2Button = new QPushButton(this);
    buttonBox->addButton(user2Button, QDialogButtonBox::ActionRole);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SelfTestDialog::reject);
    mainLayout->addWidget(buttonBox);
    user1Button->setText(i18n("Save Report..."));
    user1Button->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    user2Button->setText(i18n("Copy Report to Clipboard"));
    user2Button->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    ui.setupUi(mainWidget);

    mTestModel = new QStandardItemModel(this);
    ui.testView->setModel(mTestModel);
    connect(ui.testView->selectionModel(), &QItemSelectionModel::currentChanged, this, &SelfTestDialog::selectionChanged);
    connect(ui.detailsLabel, &QLabel::linkActivated, this, &SelfTestDialog::linkActivated);

    connect(user1Button, &QPushButton::clicked, this, &SelfTestDialog::saveReport);
    connect(user2Button, &QPushButton::clicked, this, &SelfTestDialog::copyReport);

    connect(ServerManager::self(), &ServerManager::stateChanged, this, &SelfTestDialog::runTests);
    runTests();
}

void SelfTestDialog::hideIntroduction()
{
    ui.introductionLabel->hide();
}

QStandardItem *SelfTestDialog::report(ResultType type, const KLocalizedString &summary, const KLocalizedString &details)
{
    auto item = new QStandardItem(summary.toString());
    switch (type) {
    case Skip:
        item->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok")));
        break;
    case Success:
        item->setIcon(QIcon::fromTheme(QStringLiteral("dialog-ok-apply")));
        break;
    case Warning:
        item->setIcon(QIcon::fromTheme(QStringLiteral("dialog-warning")));
        break;
    case Error:
        item->setIcon(QIcon::fromTheme(QStringLiteral("dialog-error")));
        break;
    }
    item->setEditable(false);
    item->setWhatsThis(details.toString());
    item->setData(type, ResultTypeRole);
    item->setData(summary.toString(nullptr), SummaryRole);
    item->setData(details.toString(nullptr), DetailsRole);
    mTestModel->appendRow(item);
    return item;
}

void SelfTestDialog::selectionChanged(const QModelIndex &index)
{
    if (index.isValid()) {
        ui.detailsLabel->setText(index.data(Qt::WhatsThisRole).toString());
        ui.detailsGroup->setEnabled(true);
    } else {
        ui.detailsLabel->setText(QString());
        ui.detailsGroup->setEnabled(false);
    }
}

void SelfTestDialog::runTests()
{
    mTestModel->clear();

    const QString driver = serverSetting(QStringLiteral("General"), "Driver", QStringLiteral("QMYSQL")).toString();
    testSQLDriver();
    if (driver == QLatin1String("QPSQL")) {
        testPSQLServer();
    } else {
#ifndef Q_OS_WIN
        testRootUser();
#endif
        testMySQLServer();
        testMySQLServerLog();
        testMySQLServerConfig();
    }
    testAkonadiCtl();
    testServerStatus();
    testProtocolVersion();
    testResources();
    testServerLog();
    testControlLog();
}

QVariant SelfTestDialog::serverSetting(const QString &group, const char *key, const QVariant &def) const
{
    const QString serverConfigFile = StandardDirs::serverConfigFile(StandardDirs::ReadOnly);
    QSettings settings(serverConfigFile, QSettings::IniFormat);
    settings.beginGroup(group);
    return settings.value(QString::fromLatin1(key), def);
}

bool SelfTestDialog::useStandaloneMysqlServer() const
{
    const QString driver = serverSetting(QStringLiteral("General"), "Driver", QStringLiteral("QMYSQL")).toString();
    if (driver != QLatin1String("QMYSQL")) {
        return false;
    }
    const bool startServer = serverSetting(driver, "StartServer", true).toBool();
    return startServer;
}

bool SelfTestDialog::runProcess(const QString &app, const QStringList &args, QString &result) const
{
    QProcess proc;
    proc.start(app, args);
    const bool rv = proc.waitForFinished();
    result.clear();
    result = QString::fromLocal8Bit(proc.readAllStandardError());
    result += QString::fromLocal8Bit(proc.readAllStandardOutput());
    return rv;
}

void SelfTestDialog::testSQLDriver()
{
    const QString driver = serverSetting(QStringLiteral("General"), "Driver", QStringLiteral("QMYSQL")).toString();
    const QStringList availableDrivers = QSqlDatabase::drivers();
    const KLocalizedString detailsOk =
        ki18n("The QtSQL driver '%1' is required by your current Akonadi server configuration and was found on your system.").subs(driver);
    const KLocalizedString detailsFail = ki18n(
                                             "The QtSQL driver '%1' is required by your current Akonadi server configuration.\n"
                                             "The following drivers are installed: %2.\n"
                                             "Make sure the required driver is installed.")
                                             .subs(driver)
                                             .subs(availableDrivers.join(QLatin1String(", ")));
    QStandardItem *item = nullptr;
    if (availableDrivers.contains(driver)) {
        item = report(Success, ki18n("Database driver found."), detailsOk);
    } else {
        item = report(Error, ki18n("Database driver not found."), detailsFail);
    }
    item->setData(StandardDirs::serverConfigFile(StandardDirs::ReadOnly), FileIncludeRole);
}

void SelfTestDialog::testMySQLServer()
{
    if (!useStandaloneMysqlServer()) {
        report(Skip, ki18n("MySQL server executable not tested."), ki18n("The current configuration does not require an internal MySQL server."));
        return;
    }

    const QString driver = serverSetting(QStringLiteral("General"), "Driver", QStringLiteral("QMYSQL")).toString();
    const QString serverPath = serverSetting(driver, "ServerPath", QString()).toString(); // ### default?

    const KLocalizedString details = ki18n(
                                         "You have currently configured Akonadi to use the MySQL server '%1'.\n"
                                         "Make sure you have the MySQL server installed, set the correct path and ensure you have the "
                                         "necessary read and execution rights on the server executable. The server executable is typically "
                                         "called 'mysqld'; its location varies depending on the distribution.")
                                         .subs(serverPath);

    QFileInfo info(serverPath);
    if (!info.exists()) {
        report(Error, ki18n("MySQL server not found."), details);
    } else if (!info.isReadable()) {
        report(Error, ki18n("MySQL server not readable."), details);
    } else if (!info.isExecutable()) {
        report(Error, ki18n("MySQL server not executable."), details);
    } else if (!serverPath.contains(QLatin1String("mysqld"))) {
        report(Warning, ki18n("MySQL found with unexpected name."), details);
    } else {
        report(Success, ki18n("MySQL server found."), details);
    }

    // be extra sure and get the server version while we are at it
    QString result;
    if (runProcess(serverPath, QStringList() << QStringLiteral("--version"), result)) {
        const KLocalizedString details = ki18n("MySQL server found: %1").subs(result);
        report(Success, ki18n("MySQL server is executable."), details);
    } else {
        const KLocalizedString details = ki18n("Executing the MySQL server '%1' failed with the following error message: '%2'").subs(serverPath).subs(result);
        report(Error, ki18n("Executing the MySQL server failed."), details);
    }
}

void SelfTestDialog::testMySQLServerLog()
{
    if (!useStandaloneMysqlServer()) {
        report(Skip, ki18n("MySQL server error log not tested."), ki18n("The current configuration does not require an internal MySQL server."));
        return;
    }

    const QString logFileName = StandardDirs::saveDir("data", QStringLiteral("db_data")) + QLatin1String("/mysql.err");
    const QFileInfo logFileInfo(logFileName);
    if (!logFileInfo.exists() || logFileInfo.size() == 0) {
        report(Success,
               ki18n("No current MySQL error log found."),
               ki18n("The MySQL server did not report any errors during this startup. The log can be found in '%1'.").subs(logFileName));
        return;
    }
    QFile logFile(logFileName);
    if (!logFile.open(QFile::ReadOnly | QFile::Text)) {
        report(Error,
               ki18n("MySQL error log not readable."),
               ki18n("A MySQL server error log file was found but is not readable: %1").subs(makeLink(logFileName)));
        return;
    }
    bool warningsFound = false;
    QStandardItem *item = nullptr;
    while (!logFile.atEnd()) {
        const QString line = QString::fromUtf8(logFile.readLine());
        if (line.contains(QLatin1String("error"), Qt::CaseInsensitive)) {
            item = report(Error,
                          ki18n("MySQL server log contains errors."),
                          ki18n("The MySQL server error log file '%1' contains errors.").subs(makeLink(logFileName)));
            item->setData(logFileName, FileIncludeRole);
            return;
        }
        if (!warningsFound && line.contains(QLatin1String("warn"), Qt::CaseInsensitive)) {
            warningsFound = true;
        }
    }
    if (warningsFound) {
        item = report(Warning,
                      ki18n("MySQL server log contains warnings."),
                      ki18n("The MySQL server log file '%1' contains warnings.").subs(makeLink(logFileName)));
    } else {
        item = report(Success,
                      ki18n("MySQL server log contains no errors."),
                      ki18n("The MySQL server log file '%1' does not contain any errors or warnings.").subs(makeLink(logFileName)));
    }
    item->setData(logFileName, FileIncludeRole);

    logFile.close();
}

void SelfTestDialog::testMySQLServerConfig()
{
    if (!useStandaloneMysqlServer()) {
        report(Skip, ki18n("MySQL server configuration not tested."), ki18n("The current configuration does not require an internal MySQL server."));
        return;
    }

    QStandardItem *item = nullptr;
    const QString globalConfig = StandardDirs::locateResourceFile("config", QStringLiteral("mysql-global.conf"));
    const QFileInfo globalConfigInfo(globalConfig);
    if (!globalConfig.isEmpty() && globalConfigInfo.exists() && globalConfigInfo.isReadable()) {
        item = report(Success,
                      ki18n("MySQL server default configuration found."),
                      ki18n("The default configuration for the MySQL server was found and is readable at %1.").subs(makeLink(globalConfig)));
        item->setData(globalConfig, FileIncludeRole);
    } else {
        report(Error,
               ki18n("MySQL server default configuration not found."),
               ki18n("The default configuration for the MySQL server was not found or was not readable. "
                     "Check your Akonadi installation is complete and you have all required access rights."));
    }

    const QString localConfig = StandardDirs::locateResourceFile("config", QStringLiteral("mysql-local.conf"));
    const QFileInfo localConfigInfo(localConfig);
    if (localConfig.isEmpty() || !localConfigInfo.exists()) {
        report(Skip,
               ki18n("MySQL server custom configuration not available."),
               ki18n("The custom configuration for the MySQL server was not found but is optional."));
    } else if (localConfigInfo.exists() && localConfigInfo.isReadable()) {
        item = report(Success,
                      ki18n("MySQL server custom configuration found."),
                      ki18n("The custom configuration for the MySQL server was found and is readable at %1").subs(makeLink(localConfig)));
        item->setData(localConfig, FileIncludeRole);
    } else {
        report(Error,
               ki18n("MySQL server custom configuration not readable."),
               ki18n("The custom configuration for the MySQL server was found at %1 but is not readable. "
                     "Check your access rights.")
                   .subs(makeLink(localConfig)));
    }

    const QString actualConfig = StandardDirs::saveDir("data") + QStringLiteral("/mysql.conf");
    const QFileInfo actualConfigInfo(actualConfig);
    if (actualConfig.isEmpty() || !actualConfigInfo.exists() || !actualConfigInfo.isReadable()) {
        report(Error,
               ki18n("MySQL server configuration not found or not readable."),
               ki18n("The MySQL server configuration was not found or is not readable."));
    } else {
        item = report(Success,
                      ki18n("MySQL server configuration is usable."),
                      ki18n("The MySQL server configuration was found at %1 and is readable.").subs(makeLink(actualConfig)));
        item->setData(actualConfig, FileIncludeRole);
    }
}

void SelfTestDialog::testPSQLServer()
{
    const QString dbname = serverSetting(QStringLiteral("QPSQL"), "Name", QStringLiteral("akonadi")).toString();
    const QString hostname = serverSetting(QStringLiteral("QPSQL"), "Host", QStringLiteral("localhost")).toString();
    const QString username = serverSetting(QStringLiteral("QPSQL"), "User", QString()).toString();
    const QString password = serverSetting(QStringLiteral("QPSQL"), "Password", QString()).toString();
    const int port = serverSetting(QStringLiteral("QPSQL"), "Port", 5432).toInt();

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
    db.setHostName(hostname);
    db.setDatabaseName(dbname);

    if (!username.isEmpty()) {
        db.setUserName(username);
    }

    if (!password.isEmpty()) {
        db.setPassword(password);
    }

    db.setPort(port);

    if (!db.open()) {
        const KLocalizedString details = ki18n(db.lastError().text().toLatin1().constData());
        report(Error, ki18n("Cannot connect to PostgreSQL server."), details);
    } else {
        report(Success, ki18n("PostgreSQL server found."), ki18n("The PostgreSQL server was found and connection is working."));
    }
    db.close();
}

void SelfTestDialog::testAkonadiCtl()
{
    const QString path = Akonadi::StandardDirs::findExecutable(QStringLiteral("akonadictl"));
    if (path.isEmpty()) {
        report(Error,
               ki18n("akonadictl not found"),
               ki18n("The program 'akonadictl' needs to be accessible in $PATH. "
                     "Make sure you have the Akonadi server installed."));
        return;
    }
    QString result;
    if (runProcess(path, QStringList() << QStringLiteral("--version"), result)) {
        report(Success,
               ki18n("akonadictl found and usable"),
               ki18n("The program '%1' to control the Akonadi server was found "
                     "and could be executed successfully.\nResult:\n%2")
                   .subs(path)
                   .subs(result));
    } else {
        report(Error,
               ki18n("akonadictl found but not usable"),
               ki18n("The program '%1' to control the Akonadi server was found "
                     "but could not be executed successfully.\nResult:\n%2\n"
                     "Make sure the Akonadi server is installed correctly.")
                   .subs(path)
                   .subs(result));
    }
}

void SelfTestDialog::testServerStatus()
{
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Control))) {
        report(Success,
               ki18n("Akonadi control process registered at D-Bus."),
               ki18n("The Akonadi control process is registered at D-Bus which typically indicates it is operational."));
    } else {
        report(Error,
               ki18n("Akonadi control process not registered at D-Bus."),
               ki18n("The Akonadi control process is not registered at D-Bus which typically means it was not started "
                     "or encountered a fatal error during startup."));
    }

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(ServerManager::serviceName(ServerManager::Server))) {
        report(Success,
               ki18n("Akonadi server process registered at D-Bus."),
               ki18n("The Akonadi server process is registered at D-Bus which typically indicates it is operational."));
    } else {
        report(Error,
               ki18n("Akonadi server process not registered at D-Bus."),
               ki18n("The Akonadi server process is not registered at D-Bus which typically means it was not started "
                     "or encountered a fatal error during startup."));
    }
}

void SelfTestDialog::testProtocolVersion()
{
    if (Internal::serverProtocolVersion() < 0) {
        report(Skip,
               ki18n("Protocol version check not possible."),
               ki18n("Without a connection to the server it is not possible to check if the protocol version meets the requirements."));
        return;
    }
    if (Internal::serverProtocolVersion() < Protocol::version()) {
        report(Error,
               ki18n("Server protocol version is too old."),
               ki18n("The server protocol version is %1, but version %2 is required by the client. "
                     "If you recently updated KDE PIM, please make sure to restart both Akonadi and KDE PIM applications.")
                   .subs(Internal::serverProtocolVersion())
                   .subs(Protocol::version()));
    } else if (Internal::serverProtocolVersion() > Protocol::version()) {
        report(Error,
               ki18n("Server protocol version is too new."),
               ki18n("The server protocol version is %1, but version %2 is required by the client. "
                     "If you recently updated KDE PIM, please make sure to restart both Akonadi and KDE PIM applications.")
                   .subs(Internal::serverProtocolVersion())
                   .subs(Protocol::version()));
    } else {
        report(Success, ki18n("Server protocol version matches."), ki18n("The current Protocol version is %1.").subs(Internal::serverProtocolVersion()));
    }
}

void SelfTestDialog::testResources()
{
    const AgentType::List agentTypes = AgentManager::self()->types();
    bool resourceFound = false;
    for (const AgentType &type : agentTypes) {
        if (type.capabilities().contains(QLatin1String("Resource"))) {
            resourceFound = true;
            break;
        }
    }

    const auto pathList = StandardDirs::locateAllResourceDirs(QStringLiteral("akonadi/agents"));
    QStandardItem *item = nullptr;
    if (resourceFound) {
        item = report(Success, ki18n("Resource agents found."), ki18n("At least one resource agent has been found."));
    } else {
        item = report(Error,
                      ki18n("No resource agents found."),
                      ki18n("No resource agents have been found, Akonadi is not usable without at least one. "
                            "This usually means that no resource agents are installed or that there is a setup problem. "
                            "The following paths have been searched: '%1'. "
                            "The XDG_DATA_DIRS environment variable is set to '%2'; make sure this includes all paths "
                            "where Akonadi agents are installed.")
                          .subs(pathList.join(QLatin1Char(' ')))
                          .subs(QString::fromLocal8Bit(qgetenv("XDG_DATA_DIRS"))));
    }
    item->setData(pathList, ListDirectoryRole);
    item->setData(QByteArray("XDG_DATA_DIRS"), EnvVarRole);
}

void SelfTestDialog::testServerLog()
{
    QString serverLog = StandardDirs::saveDir("data") + QLatin1String("/akonadiserver.error");
    QFileInfo info(serverLog);
    if (!info.exists() || info.size() <= 0) {
        report(Success, ki18n("No current Akonadi server error log found."), ki18n("The Akonadi server did not report any errors during its current startup."));
    } else {
        QStandardItem *item =
            report(Error,
                   ki18n("Current Akonadi server error log found."),
                   ki18n("The Akonadi server reported errors during its current startup. The log can be found in %1.").subs(makeLink(serverLog)));
        item->setData(serverLog, FileIncludeRole);
    }

    serverLog += QStringLiteral(".old");
    info.setFile(serverLog);
    if (!info.exists() || info.size() <= 0) {
        report(Success,
               ki18n("No previous Akonadi server error log found."),
               ki18n("The Akonadi server did not report any errors during its previous startup."));
    } else {
        QStandardItem *item =
            report(Error,
                   ki18n("Previous Akonadi server error log found."),
                   ki18n("The Akonadi server reported errors during its previous startup. The log can be found in %1.").subs(makeLink(serverLog)));
        item->setData(serverLog, FileIncludeRole);
    }
}

void SelfTestDialog::testControlLog()
{
    QString controlLog = StandardDirs::saveDir("data") + QLatin1String("/akonadi_control.error");
    QFileInfo info(controlLog);
    if (!info.exists() || info.size() <= 0) {
        report(Success,
               ki18n("No current Akonadi control error log found."),
               ki18n("The Akonadi control process did not report any errors during its current startup."));
    } else {
        QStandardItem *item =
            report(Error,
                   ki18n("Current Akonadi control error log found."),
                   ki18n("The Akonadi control process reported errors during its current startup. The log can be found in %1.").subs(makeLink(controlLog)));
        item->setData(controlLog, FileIncludeRole);
    }

    controlLog += QStringLiteral(".old");
    info.setFile(controlLog);
    if (!info.exists() || info.size() <= 0) {
        report(Success,
               ki18n("No previous Akonadi control error log found."),
               ki18n("The Akonadi control process did not report any errors during its previous startup."));
    } else {
        QStandardItem *item =
            report(Error,
                   ki18n("Previous Akonadi control error log found."),
                   ki18n("The Akonadi control process reported errors during its previous startup. The log can be found in %1.").subs(makeLink(controlLog)));
        item->setData(controlLog, FileIncludeRole);
    }
}

void SelfTestDialog::testRootUser()
{
    KUser user;
    if (user.isSuperUser()) {
        report(Error,
               ki18n("Akonadi was started as root"),
               ki18n("Running Internet-facing applications as root/administrator exposes you to many security risks. MySQL, used by this Akonadi installation, "
                     "will not allow itself to run as root, to protect you from these risks."));
    } else {
        report(Success,
               ki18n("Akonadi is not running as root"),
               ki18n("Akonadi is not running as a root/administrator user, which is the recommended setup for a secure system."));
    }
}

QString SelfTestDialog::createReport()
{
    QString result;
    QTextStream s(&result);
    s << "Akonadi Server Self-Test Report";
    s << "===============================";

    for (int i = 0; i < mTestModel->rowCount(); ++i) {
        QStandardItem *item = mTestModel->item(i);
        s << '\n';
        s << "Test " << (i + 1) << ":  ";
        switch (item->data(ResultTypeRole).toInt()) {
        case Skip:
            s << "SKIP";
            break;
        case Success:
            s << "SUCCESS";
            break;
        case Warning:
            s << "WARNING";
            break;
        case Error:
        default:
            s << "ERROR";
            break;
        }
        s << "\n--------\n";
        s << '\n';
        s << item->data(SummaryRole).toString() << '\n';
        s << "Details: " << item->data(DetailsRole).toString() << '\n';
        if (item->data(FileIncludeRole).isValid()) {
            s << '\n';
            const QString fileName = item->data(FileIncludeRole).toString();
            QFile f(fileName);
            if (f.open(QFile::ReadOnly)) {
                s << "File content of '" << fileName << "':" << '\n';
                s << f.readAll() << '\n';
            } else {
                s << "File '" << fileName << "' could not be opened\n";
            }
        }
        if (item->data(ListDirectoryRole).isValid()) {
            s << '\n';
            const QStringList pathList = item->data(ListDirectoryRole).toStringList();
            if (pathList.isEmpty()) {
                s << "Directory list is empty.\n";
            }
            for (const QString &path : pathList) {
                s << "Directory listing of '" << path << "':\n";
                QDir dir(path);
                dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
                const QStringList listEntries(dir.entryList());
                for (const QString &entry : listEntries) {
                    s << entry << '\n';
                }
            }
        }
        if (item->data(EnvVarRole).isValid()) {
            s << '\n';
            const QByteArray envVarName = item->data(EnvVarRole).toByteArray();
            const QByteArray envVarValue = qgetenv(envVarName.constData());
            s << "Environment variable " << envVarName << " is set to '" << envVarValue << "'\n";
        }
    }

    s << '\n';
    s.flush();
    return result;
}

void SelfTestDialog::saveReport()
{
    const QString defaultFileName =
        QStringLiteral("akonadi-selftest-report-") + QDate::currentDate().toString(QStringLiteral("yyyyMMdd")) + QStringLiteral(".txt");
    const QString fileName = QFileDialog::getSaveFileName(this, i18n("Save Test Report"), defaultFileName);
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QFile::ReadWrite)) {
        QMessageBox::critical(this, i18n("Error"), i18n("Could not open file '%1'", fileName));
        return;
    }

    file.write(createReport().toUtf8());
    file.close();
}

void SelfTestDialog::copyReport()
{
#ifndef QT_NO_CLIPBOARD
    QApplication::clipboard()->setText(createReport());
#endif
}

void SelfTestDialog::linkActivated(const QString &link)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(link));
}

/// @endcond
