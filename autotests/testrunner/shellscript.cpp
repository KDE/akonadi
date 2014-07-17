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

#include "shellscript.h"

#include "config.h" //krazy:exclude=includes

#include <QDebug>
#include <QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QHashIterator>

ShellScript::ShellScript()
{
}

void ShellScript::writeEnvironmentVariables()
{
  foreach ( const EnvVar &envvar, mEnvVars ) {
    mScript += "_old_" + envvar.first + "=" + envvar.first + "\n";
    mScript.append( envvar.first );
    mScript.append( QLatin1Char( '=' ) );
    mScript.append( envvar.second );
    mScript.append( QLatin1Char( '\n' ) );

    mScript.append( QLatin1String( "export " ) );
    mScript.append( envvar.first );
    mScript.append( QLatin1Char( '\n' ) );
  }

  mScript.append( QLatin1String( "\n\n" ) );
}

void ShellScript::writeShutdownFunction()
{
  QString s =
    "function shutdown-testenvironment()\n"
    "{\n"
    "  qdbus org.kde.Akonadi.Testrunner-" + QString::number( QCoreApplication::instance()->applicationPid() ) + " / org.kde.Akonadi.Testrunner.shutdown\n";

  foreach ( const EnvVar &envvar, mEnvVars ) {
    s += "  " + envvar.first + "=$_old_" + envvar.first + "\n";
    s += "  export " + envvar.first + "\n";
  }
  s.append( "}\n\n" );
  mScript.append( s );
}

void ShellScript::makeShellScript( const QString &fileName )
{
  qDebug() << fileName;
  QFile file( fileName ); //can user define the file name/location?

  if ( file.open( QIODevice::WriteOnly ) ) {
    writeEnvironmentVariables();
    writeShutdownFunction();

    file.write( mScript.toLatin1(), qstrlen( mScript.toAscii() ) );
    file.close();
  } else {
    qCritical() << "Failed to write" << fileName;
  }
}

void ShellScript::setEnvironmentVariables(const QVector< ShellScript::EnvVar >& envVars)
{
  mEnvVars = envVars;
}
