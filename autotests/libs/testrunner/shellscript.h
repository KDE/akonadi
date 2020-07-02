/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef SHELLSCRIPT_H
#define SHELLSCRIPT_H

#include <QString>
#include <QVector>
#include <QPair>

class ShellScript
{
public:
    ShellScript();
    void makeShellScript(const QString &filename);

    typedef QPair<QByteArray, QByteArray> EnvVar;
    void setEnvironmentVariables(const QVector<EnvVar> &envVars);

private:
    void writeEnvironmentVariables();
    void writeShutdownFunction();

    QString mScript;
    QVector<EnvVar> mEnvVars;
};
#endif
