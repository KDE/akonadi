/*
 * SPDX-FileCopyrightText: 2009 Volker Krause <vkrause@kde.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include <QObject>
#include <QProcess>
#include <QStringList>

class KProcess;

class TestRunner : public QObject
{
    Q_OBJECT

public:
    explicit TestRunner(const QStringList &args, QObject *parent = nullptr);
    int exitCode() const;
    void terminate();

public Q_SLOTS:
    void run();
    void triggerTermination(int);

Q_SIGNALS:
    void finished();

private Q_SLOTS:
    void processFinished(int exitCode);
    void processError(QProcess::ProcessError error);

private:
    QStringList mArguments;
    int mExitCode;
    KProcess *mProcess = nullptr;
};

#endif // TESTRUNNER_H
