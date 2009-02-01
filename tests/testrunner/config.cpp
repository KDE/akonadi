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

Config* Config::mInstance = 0;

Config::Config()
{
}

Config *Config::instance( const QString& pathToConfig )
{
  if ( mInstance == 0 ) {
    QString cfgFile = pathToConfig;
    if ( cfgFile.isEmpty() )
       cfgFile = KStandardDirs::locate( "config", "akonaditest.xml" );

     mInstance = new ConfigReader( cfgFile );
  }

  return mInstance;
}


void Config::destroyInstance()
{
  delete mInstance;
}

QString Config::kdeHome() const
{
  return mKdeHome;
}

QString Config::xdgDataHome() const
{
  return mXdgDataHome;
}

QString Config::xdgConfigHome() const
{
  return mXdgConfigHome;
}

void Config::setKdeHome( const QString &home )
{
  const QDir kdeHomeDir( home );
  mKdeHome = kdeHomeDir.absolutePath();
}

void Config::setXdgDataHome( const QString &dataHome )
{
  const QDir dataHomeDir( dataHome );
  mXdgDataHome = dataHomeDir.absolutePath();
}

void Config::setXdgConfigHome( const QString &configHome )
{
  const QDir configHomeDir( configHome );
  mXdgConfigHome = configHomeDir.absolutePath();
}

void Config::insertItemConfig( const QString &itemName, const QString &collectionName )
{
  mItemConfig.append( qMakePair( itemName, collectionName ) );
}

QList< QPair<QString, QString> > Config::itemConfig() const
{
  return mItemConfig;
}

void Config::insertAgent( const QString &agent, bool sync )
{
  mAgents.append( qMakePair( agent, sync ) );
}

QList<QPair<QString, bool> > Config::agents() const
{
  return mAgents;
}
