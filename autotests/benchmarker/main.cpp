/*
    Copyright (c) 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>
    based on kdepimlibs/akonadi/tests/benchmarker.cpp wrote by Robert Zwerus <arzie@dds.nl>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "testmaildir.h"
#include "testvcard.h"
#include <kcmdlineargs.h>
#include <kapplication.h>
#include <klocale.h>

int main(int argc, char *argv[])
{
  KCmdLineArgs::init( argc, argv, "benchmarker", 0, ki18n("Benchmarker") ,"1.0" ,ki18n("benchmark application") );

  KCmdLineOptions options;
  options.add("maildir <argument>", ki18n("Path to maildir to be used as data source"));
  options.add("vcarddir <argument>", ki18n("Path to vvcarddir to be used as data source"));
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  const QString maildir = args->getOption( "maildir" );
  const QString vcarddir = args->getOption( "vcarddir" );

  TestMailDir *mailDirTest = new TestMailDir(maildir);
  TestVCard *vcardTest = new TestVCard(vcarddir);

  mailDirTest->runTests();
  vcardTest->runTests();

  return app.exec();
}
