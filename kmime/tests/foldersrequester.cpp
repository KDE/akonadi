/*
    Copyright (c) 2009 Constantin Berzan <exit3219@gmail.com>

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

#include "foldersrequester.h"

#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>

#include <akonadi/collection.h>
#include <akonadi/control.h>
#include <akonadi/kmime/localfolders.h>

using namespace Akonadi;


Requester::Requester()
{
  Control::start();

  connect( LocalFolders::self(), SIGNAL( foldersReady() ),
      this, SLOT( checkFolders() ) );
  LocalFolders::self()->fetch();
}

void Requester::checkFolders()
{
  Collection outbox = LocalFolders::self()->outbox();
  Collection sentMail = LocalFolders::self()->sentMail();

  kDebug() << "Got outbox" << outbox.id() << "sent-mail" << sentMail.id();

  if( !outbox.isValid() || !sentMail.isValid() ) {
    KApplication::exit( 1 );
  } else {
    KApplication::exit( 2 );
  }
}

int main( int argc, char **argv )
{
  KCmdLineArgs::init( argc, argv, "foldersrequester", 0,
                      ki18n( "foldersrequester" ), "0",
                      ki18n( "An app that requests LocalFolders" ) );
  KApplication app;
  new Requester();
  return app.exec();
}


#include "foldersrequester.moc"
