/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QList>
#include <QString>
#include <utility>

class ShellScript
{
public:
    ShellScript();
    void makeShellScript(const QString &filename);

    using EnvVar = std::pair<QByteArray, QByteArray>;
    void setEnvironmentVariables(const QList<EnvVar> &envVars);

private:
    void writeEnvironmentVariables();
    void writeShutdownFunction();

    QString mScript;
    QList<EnvVar> mEnvVars;
};
