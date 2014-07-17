/*
    Copyright (c) 2010 Stephen Kelly <steveire@gmail.com>

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

#include <KApplication>
#include <KLocalizedString>
#include <KAboutData>
#include <KCmdLineArgs>

#include "mainwindow.h"

int main(int argc, char* argv[])
{
  KAboutData aboutData( "etm_test_app", 0,
                        ki18n( "ETM Test application" ),
                        "0.99",
                        ki18n( "Test app for EntityTreeModel" ),
                        KAboutData::License_GPL,
                        ki18n( "" ),
                        KLocalizedString(),
                        "http://pim.kde.org/akonadi/" );
  aboutData.setProgramIconName( "akonadi" );
  aboutData.addAuthor( ki18n( "Stephen Kelly" ), ki18n( "Author" ), "steveire@gmail.com" );

  KCmdLineArgs::init( argc, argv, &aboutData );
  KApplication app;

  MainWindow mw;
  mw.show();

  return app.exec();
}

