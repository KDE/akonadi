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
#include "shellscript.h"
#include <QHashIterator>
#include <QFile>
#include <QtTest>

shellScript::shellScript()
{
  symbol = Symbols::getInstance();
}

void shellScript::writeEnvironmentVariables()
{
  QHashIterator<QString, QString> i( symbol->getSymbols() );

  while( i.hasNext() )
  {
    i.next();
    script.append( i.key() );
    script.append( "=" );
    script.append( i.value() );
    script.append( "\n" );

    script.append("export ");
    script.append( i.key() );
    script.append("\n");
  }
  script.append("\n\n");
}

void shellScript::writeShutdownFunction()
{
  const QString s =
    "function shutdown-testenvironment()\n"
    "{\n"
    "  echo Stopping Akonadi server\n"
    "  qdbus org.freedesktop.Akonadi.Control /ControlManager org.freedesktop.Akonadi.ControlManager.shutdown\n"
    "  echo \"Stopping testrunner with PID \" $AKONADI_TESTRUNNER_PID\n"
    "  kill $AKONADI_TESTRUNNER_PID\n"
    "  # wait a bit before killing D-Bus\n"
    "  echo \"Waiting 10 seconds before killing D-Bus\"\n"
    "  sleep 10\n"
    "  echo \"Killing D-Bus with PID \" $DBUS_SESSION_BUS_PID\n"
    "  kill $DBUS_SESSION_BUS_PID\n"
    "  rm -fr /tmp/akonadi_testrunner\n"
    "}\n\n";
  script.append( s );
}

void shellScript::makeShellScript(const QString &filename)
{
  QFile file(filename); //can user define the file name/location?

  file.open( QIODevice::WriteOnly );

  writeEnvironmentVariables();
  writeShutdownFunction();

  file.write(script.toAscii(), qstrlen(script.toAscii()) );
  file.close();
}

