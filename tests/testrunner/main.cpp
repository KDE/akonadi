/*people
 *
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

#include <KApplication>
#include <KAboutData>
#include <KCmdLineArgs>
#include <QtTest>
#include "akonaditesting.h"
#include "setup.h"
#include "config.h"
#include "shellscript.h"

int main(int argc, char **argv) {
  KAboutData aboutdata("akonadi-TES", 0,
          ki18n("Akonadi Testing Environment Setup"),
          "1.0",
          ki18n("Setup Environmnet"),
          KAboutData::License_GPL,
          ki18n("(c) 2008 Igor Trindade Oliveira"));

  KCmdLineArgs::init(argc, argv, &aboutdata);

  KCmdLineOptions options;

  // TODO: add --config option for overriding with a specific one

  KCmdLineArgs::addCmdLineOptions(options);

  KApplication app;

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  Q_UNUSED( args ) // TODO pass value of --config once available
  SetupTest *setup = new SetupTest();

  setup->startAkonadiDaemon();

  AkonadiTesting *testing = new AkonadiTesting();

  testing->insertItemFromList();

  shellScript *sh = new shellScript();
  sh->makeShellScript();

  int result = app.exec();

  Config::destroyInstance();
  delete testing;
  delete setup;
  delete sh;

  return result;
}

