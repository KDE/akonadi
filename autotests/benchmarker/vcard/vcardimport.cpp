/*
    Copyright (c) 2009 Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

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

#include "vcardimport.h"
#include "vcard.h"

#include <QDebug>

#define WAIT_TIME 50

VCardImport::VCardImport(const QString &dir) : VCard(dir) {}

void VCardImport::runTest() {
  done = false;
  timer.start();
  qDebug() << "Synchronising resource";
  currentInstance.synchronize();
  while(!done)
     QTest::qWait( WAIT_TIME );
  outputStats( "import");
}
