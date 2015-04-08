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

#include <QtCore/QDir>
#include <QtCore/QPair>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtXml/QDomDocument>
#include <QtXml/QDomElement>
#include <QtXml/QDomNode>

Q_GLOBAL_STATIC(Config, globalConfig)

Config::Config()
{
}

Config::~Config()
{
}

Config *Config::instance(const QString &pathToConfig)
{
    if (!pathToConfig.isEmpty()) {
        globalConfig()->readConfiguration(pathToConfig);
    }

    return globalConfig();
}

void Config::readConfiguration(const QString &configfile)
{
    QDomDocument doc;
    QFile file(configfile);

    if (!file.open(QIODevice::ReadOnly)) {
        qFatal("error reading file: %s", qPrintable(configfile));
    }

    QString errorMsg;
    if (!doc.setContent(&file, &errorMsg)) {
        qFatal("unable to parse config file: %s", qPrintable(errorMsg));
    }

    const QDomElement root = doc.documentElement();
    if (root.tagName() != QLatin1String("config")) {
        qFatal("could not file root tag");
    }

    mBasePath = QFileInfo(configfile).absolutePath() + QLatin1Char('/');

    QDomNode node = root.firstChild();
    while (!node.isNull()) {
        const QDomElement element = node.toElement();
        if (!element.isNull()) {
            if (element.tagName() == QLatin1String("confighome")) {
                setXdgConfigHome(mBasePath + element.text());
            } else if (element.tagName() == QLatin1String("datahome")) {
                setXdgDataHome(mBasePath + element.text());
            } else if (element.tagName() == QLatin1String("agent")) {
                insertAgent(element.text(), element.attribute(QLatin1String("synchronize"), QLatin1String("false")) == QLatin1String("true"));
            } else if (element.tagName() == QLatin1String("envvar")) {
                const QString name = element.attribute(QLatin1String("name"));
                if (name.isEmpty()) {
                    qWarning() << "Given envvar with no name.";
                } else {
                    mEnvVars[name] = element.text();
                }
            }
        }

        node = node.nextSibling();
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
