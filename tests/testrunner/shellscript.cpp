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

void shellScript::makeShellScript()
{
  QFile file("testenvironment.sh"); //can user define the file name/location?

  file.open( QIODevice::WriteOnly );
  
  writeEnvironmentVariables();
  
  //script.append("exec /usr/bin/dbus-launch \n");
  //script.append("exec akonadiconsole\n"); 
  file.write(script.toAscii(), qstrlen(script.toAscii()) );
  file.close();
}


