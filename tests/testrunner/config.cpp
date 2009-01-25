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

#include "config.h"
#include "configreader.h"

#include <kstandarddirs.h>

#include <QtCore/QDir>
#include <QtCore/QPair>

Config* Config::instance = 0;

Config::Config()
{
}

Config *Config::getInstance( const QString& pathToConfig )
{
  if ( instance == 0 ) {
    QString cfgFile = pathToConfig;
    if ( cfgFile.isEmpty() )
       cfgFile = KStandardDirs::locate( "config", "akonaditest.xml" );

     instance = new ConfigReader( cfgFile );
  }

  return instance;
}


void Config::destroyInstance()
{
  delete instance;
}

QString Config::getKdeHome() const
{
  return kdehome;
}

QString Config::getXdgDataHome() const
{
  return xdgdatahome;
}

QString Config::getXdgConfigHome() const
{
  return xdgconfighome;
}

void Config::setKdeHome( const QString &home )
{
  const QDir kdeHomeDir( home );
  kdehome = kdeHomeDir.absolutePath();
}

void Config::setXdgDataHome( const QString &datahome )
{
  const QDir dataHomeDir( datahome );
  xdgdatahome = dataHomeDir.absolutePath();
}

void Config::setXdgConfigHome( const QString &confighome )
{
  const QDir configHomeDir( confighome );
  xdgconfighome = configHomeDir.absolutePath();
}

void Config::insertItemConfig( const QString &itemname, const QString &colname )
{
  itemconfig.append( qMakePair( itemname, colname ) );
}

QList< QPair<QString, QString> > Config::getItemConfig() const
{
  return itemconfig;
}

void Config::insertAgent( const QString &agent )
{
  mAgents << agent;
}

QStringList Config::getAgents() const
{
  return mAgents;
}
