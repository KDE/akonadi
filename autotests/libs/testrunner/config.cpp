/*
 * Copyright (c) 2008  Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h" //krazy:exclude=includes

#include <QDebug>

#include <QDir>
#include <QPair>
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>

Q_GLOBAL_STATIC(Config, globalConfig)

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
        qFatal("error reading file: %s", qPrintable(configfile));
    }

    mBasePath = QFileInfo(configfile).absolutePath() + QLatin1Char('/');
    qDebug() << "Base path" << mBasePath;
    QXmlStreamReader reader(&file);

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.name() == QLatin1String("config")) {
            while (!reader.atEnd() && !(reader.name() == QLatin1String("config") && reader.isEndElement())) {
                reader.readNext();
                if (reader.name() == QLatin1String("backends")) {
                    QStringList backends;
                    while (!reader.atEnd() && !(reader.name() == QLatin1String("backends") && reader.isEndElement())) {
                        reader.readNext();
                        if (reader.name() == QLatin1String("backend")) {
                            backends << reader.readElementText();
                        }
                    }
                    setBackends(backends);
                } else if (reader.name() == QLatin1String("datahome")) {
                    setXdgDataHome(mBasePath + reader.readElementText());
                } else if (reader.name() == QLatin1String("agent")) {
                    const auto attrs = reader.attributes();
                    insertAgent(reader.readElementText(), attrs.value(QLatin1String("synchronize")) == QLatin1String("true"));
                } else if (reader.name() == QLatin1String("envvar")) {
                    const auto attrs = reader.attributes();
                    const auto name = attrs.value(QLatin1String("name"));
                    if (name.isEmpty()) {
                        qWarning() << "Given envvar with no name.";
                    } else {
                        mEnvVars[name.toString()] = reader.readElementText();
                    }
                } else if (reader.name() == QLatin1String("dbbackend")) {
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

QList<QPair<QString, bool> > Config::agents() const
{
    return mAgents;
}

QHash<QString, QString> Config::envVars() const
{
    return mEnvVars;
}
