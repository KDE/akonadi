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

#ifndef CONFIG_H
#define CONFIG_H

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

class Config
{
  public:
    Config();
    ~Config();
    static Config *instance( const QString &pathToConfig = QString() );
    static void destroyInstance();
    QString kdeHome() const;
    QString xdgDataHome() const;
    QString xdgConfigHome() const;
    QList<QPair<QString, bool> > agents() const;
    QHash<QString, QString> envVars() const;

  protected:
    void setKdeHome( const QString &home );
    void setXdgDataHome( const QString &dataHome );
    void setXdgConfigHome( const QString &configHome );
    void insertAgent( const QString &agent, bool sync );

  private:
    void readConfiguration(const QString &configFile);

  private:
    QString mKdeHome;
    QString mXdgDataHome;
    QString mXdgConfigHome;
    QList<QPair<QString, bool> > mAgents;
    QHash<QString, QString> mEnvVars;
};

#endif
