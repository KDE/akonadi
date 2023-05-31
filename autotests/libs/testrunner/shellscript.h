/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include <QList>
#include <QPair>
#include <QString>

class ShellScript
{
public:
    ShellScript();
    void makeShellScript(const QString &filename);

    using EnvVar = QPair<QByteArray, QByteArray>;
    void setEnvironmentVariables(const QList<EnvVar> &envVars);

private:
    void writeEnvironmentVariables();
    void writeShutdownFunction();

    QString mScript;
    QList<EnvVar> mEnvVars;
};
