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

#include "config.h"
#include "symbols.h"

#include <KDebug>
#include <QtCore/QFile>
#include <QtCore/QHashIterator>

ShellScript::ShellScript()
{
  mSymbol = Symbols::instance();
}

void ShellScript::writeEnvironmentVariables()
{
  QHashIterator<QString, QString> it( mSymbol->symbols() );

  while ( it.hasNext() ) {
    it.next();
    mScript.append( QString::fromLatin1("_old_%1=$%2\n").arg( it.key(), it.key() ) );
    mScript.append( it.key() );
    mScript.append( QLatin1Char( '=' ) );
    mScript.append( it.value() );
    mScript.append( QLatin1Char( '\n' ) );

    mScript.append( QLatin1String( "export " ) );
    mScript.append( it.key() );
    mScript.append( QLatin1Char( '\n' ) );
  }

  mScript.append( QLatin1String( "\n\n" ) );
}

void ShellScript::writeShutdownFunction()
{
  QString s =
    "function shutdown-testenvironment()\n"
    "{\n"
    "  qdbus org.kde.Akonadi.Testrunner / org.kde.Akonadi.Testrunner.shutdown\n";
  QHashIterator<QString, QString> it( mSymbol->symbols() );
  while ( it.hasNext() ) {
    it.next();
    s.append( QString::fromLatin1( "  %1=$_old_%2\n" ).arg( it.key(), it.key() ) );
    s.append( QString::fromLatin1( "  export %1\n" ).arg( it.key() ) );
  }
  s.append( "}\n\n" );
  mScript.append( s );
}

void ShellScript::makeShellScript( const QString &fileName )
{
  kDebug() << fileName;
  QFile file( fileName ); //can user define the file name/location?

  if ( file.open( QIODevice::WriteOnly ) ) {
    writeEnvironmentVariables();
    writeShutdownFunction();

    file.write( mScript.toAscii(), qstrlen( mScript.toAscii() ) );
    file.close();
  } else {
    kError() << "Failed to write" << fileName;
  }
}

