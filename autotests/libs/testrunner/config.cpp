/*
 * SPDX-FileCopyrightText: 2008 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "config.h" //krazy:exclude=includes
#include "akonaditest_debug.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPair>
#include <QXmlStreamReader>

Q_GLOBAL_STATIC(Config, globalConfig) // NOLINT(readability-redundant-member-init)

Config::Config()
{
}

Config::~Config()
{
}

Config *Config::instance(const QString &pathToConfig)
{
    globalConfig()->readConfiguration(pathToConfig);
    return globalConfig();
}

Config *Config::instance()
{
    return globalConfig();
}

void Config::readConfiguration(const QString &configfile)
{
    QFile file(configfile);

    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("Error reading file %s: %s", qPrintable(configfile), qUtf8Printable(file.errorString()));
    }

    mBasePath = QFileInfo(configfile).absolutePath() + QLatin1Char('/');
    qCDebug(AKONADITEST_LOG) << "Base path" << mBasePath;
    QXmlStreamReader reader(&file);

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.name() == QLatin1StringView("config")) {
            while (!reader.atEnd() && !(reader.name() == QLatin1StringView("config") && reader.isEndElement())) {
                reader.readNext();
                if (reader.name() == QLatin1StringView("backends")) {
                    QStringList backends;
                    while (!reader.atEnd() && !(reader.name() == QLatin1StringView("backends") && reader.isEndElement())) {
                        reader.readNext();
                        if (reader.name() == QLatin1StringView("backend")) {
                            backends << reader.readElementText();
                        }
                    }
                    setBackends(backends);
                } else if (reader.name() == QLatin1StringView("datahome")) {
                    setXdgDataHome(mBasePath + reader.readElementText());
                } else if (reader.name() == QLatin1StringView("agent")) {
                    const auto attrs = reader.attributes();
                    insertAgent(reader.readElementText(), attrs.value(QLatin1StringView("synchronize")) == QLatin1StringView("true"));
                } else if (reader.name() == QLatin1StringView("envvar")) {
                    const auto attrs = reader.attributes();
                    const auto name = attrs.value(QLatin1StringView("name"));
                    if (name.isEmpty()) {
                        qCWarning(AKONADITEST_LOG) << "Given envvar with no name.";
                    } else {
                        mEnvVars[name.toString()] = reader.readElementText();
                    }
                } else if (reader.name() == QLatin1StringView("dbbackend")) {
                    setDbBackend(reader.readElementText());
                }
            }
        }
    }
}

QString Config::xdgDataHome() const
{
    return mXdgDataHome;
}

QString Config::xdgConfigHome() const
{
    return mXdgConfigHome;
}

QString Config::basePath() const
{
    return mBasePath;
}

QStringList Config::backends() const
{
    return mBackends;
}

QString Config::dbBackend() const
{
    return mDbBackend;
}

void Config::setXdgDataHome(const QString &dataHome)
{
    const QDir dataHomeDir(dataHome);
    mXdgDataHome = dataHomeDir.absolutePath();
}

void Config::setXdgConfigHome(const QString &configHome)
{
    const QDir configHomeDir(configHome);
    mXdgConfigHome = configHomeDir.absolutePath();
}

void Config::setBackends(const QStringList &backends)
{
    mBackends = backends;
}

bool Config::setDbBackend(const QString &backend)
{
    if (mBackends.isEmpty() || mBackends.contains(backend)) {
        mDbBackend = backend;
        return true;
    } else {
        return false;
    }
}

void Config::insertAgent(const QString &agent, bool sync)
{
    mAgents.append(qMakePair(agent, sync));
}

QList<QPair<QString, bool>> Config::agents() const
{
    return mAgents;
}

QHash<QString, QString> Config::envVars() const
{
    return mEnvVars;
}
