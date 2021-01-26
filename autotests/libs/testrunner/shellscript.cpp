/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "shellscript.h"
#include "akonaditest_debug.h"
#include "config.h" //krazy:exclude=includes

#include <QCoreApplication>
#include <QFile>
#include <QHashIterator>

ShellScript::ShellScript()
{
}

void ShellScript::writeEnvironmentVariables()
{
    for (const auto &envvar : qAsConst(mEnvVars)) {
#ifdef Q_OS_WIN
        const auto tmpl = QStringLiteral(
            "$env:_old_%1=$env:%1\r\n"
            "$env:%1=\"%2\"\r\n");
#else
        const auto tmpl = QStringLiteral(
            "_old_%1=$%1\n"
            "%1=\"%2\"\n"
            "export %1\n");
#endif
        mScript += tmpl.arg(QString::fromLocal8Bit(envvar.first), QString::fromLocal8Bit(envvar.second).replace(QLatin1Char('"'), QStringLiteral("\\\"")));
    }

#ifdef Q_OS_WIN
    mScript += QStringLiteral("\r\n\r\n");
#else
    mScript += QStringLiteral("\n\n");
#endif
}

void ShellScript::writeShutdownFunction()
{
#ifdef Q_OS_WIN
    const auto tmpl = QStringLiteral(
        "Function shutdownTestEnvironment()\r\n"
        "{\r\n"
        "  qdbus %1 %2 %3\r\n"
        "%4"
        "}\r\n\r\n");
    const auto restoreTmpl = QStringLiteral("  $env:%1=$env:_old_%1\r\n");
#else
    const auto tmpl = QStringLiteral(
        "function shutdown-testenvironment()\n"
        "{\n"
        "  qdbus %1 %2 %3\n"
        "%4"
        "}\n\n");
    const auto restoreTmpl = QStringLiteral(
        "  %1=$_old_%1\n"
        "  export %1\n");
#endif
    QString restore;
    for (const auto &envvar : qAsConst(mEnvVars)) {
        restore += restoreTmpl.arg(QString::fromLocal8Bit(envvar.first));
    }

    mScript += tmpl.arg(QStringLiteral("org.kde.Akonadi.Testrunner-%1").arg(qApp->applicationPid()),
                        QStringLiteral("/"),
                        QStringLiteral("org.kde.Akonadi.Testrunner.shutdown"),
                        restore);
}

void ShellScript::makeShellScript(const QString &fileName)
{
    qCDebug(AKONADITEST_LOG) << "Writing environment shell script to" << fileName;
    QFile file(fileName); // can user define the file name/location?

    if (file.open(QIODevice::WriteOnly)) {
        writeEnvironmentVariables();
        writeShutdownFunction();

        file.write(mScript.toLatin1());
        file.close();
    } else {
        qCritical() << "Failed to write" << fileName;
    }
}

void ShellScript::setEnvironmentVariables(const QVector<ShellScript::EnvVar> &envVars)
{
    mEnvVars = envVars;
}
