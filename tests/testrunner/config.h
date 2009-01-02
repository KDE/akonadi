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

#include <QString>
#include <QHash>
#include <QStringList>

class Config
{
  private:
    QString kdehome;
    QString xdgdatahome;
    QString xdgconfighome;
    QList <QPair <QString, QString> >  itemconfig; 
    QStringList mAgents;
    static Config *instance;

  public:
    static Config *getInstance(); //to maintain the compatibility
    static Config *getInstance(const QString &pathToConfig);
    static void destroyInstance();
    QString getKdeHome() const;
    QString getXdgDataHome() const;
    QString getXdgConfigHome() const;
    QList<QPair<QString, QString> > getItemConfig();
    QStringList getAgents();

  protected:
    Config();
    void setKdeHome(const QString &home);
    void setXdgDataHome(const QString &datahome);
    void setXdgConfigHome(const QString &confighome);
    void insertItemConfig(const QString &itemname, const QString &colname);
    void insertAgent(QString agent);

};

#endif
