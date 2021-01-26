/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <QHash>
#include <QString>
#include <QStringList>

class Config
{
public:
    Config();
    ~Config();
    static Config *instance();
    static Config *instance(const QString &pathToConfig);
    static void destroyInstance();
    QString xdgDataHome() const;
    QString xdgConfigHome() const;
    QString basePath() const;
    QStringList backends() const;
    bool setDbBackend(const QString &backend);
    QString dbBackend() const;
    QList<QPair<QString, bool>> agents() const;
    QHash<QString, QString> envVars() const;

protected:
    void setXdgDataHome(const QString &dataHome);
    void setXdgConfigHome(const QString &configHome);
    void setBackends(const QStringList &backends);
    void insertAgent(const QString &agent, bool sync);

private:
    void readConfiguration(const QString &configFile);

private:
    QString mBasePath;
    QString mXdgDataHome;
    QString mXdgConfigHome;
    QStringList mBackends;
    QString mDbBackend;
    QList<QPair<QString, bool>> mAgents;
    QHash<QString, QString> mEnvVars;
};

#endif
