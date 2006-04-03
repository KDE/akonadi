/*
    Copyright (c) 2006 Volker Krause <volker.krause@rwth-aachen.de>

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

#include "messagebrowser.h"
#include "messagemodel.h"

#include <kapplication.h>
#include <kcmdlineargs.h>

using namespace PIM;

MessageBrowser::MessageBrowser() : QTreeView()
{
  MessageModel* model = new MessageModel( this );
  model->setQuery( "collection=inbox" );
  setModel( model );
}

int main( int argc, char** argv )
{
  KCmdLineArgs::init( argc, argv, "test", "Test" ,"test app" ,"1.0" );
  KApplication app;
  MessageBrowser w;
  w.show();
  return app.exec();
}

#include "messagebrowser.moc"
