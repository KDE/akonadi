/*
    Copyright (c) Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

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

#include "collection.h"
#include <akonadi-xml.h>

#include <qtest_kde.h>

QTEST_KDEMAIN(CollectionTest, NoGUI)

 QByteArray collection1(
"<collection rid=\"c11\" name=\"Inbox\" content=\"inode/directory,message/rfc822\">           \
      <attribute type=\"ENTITYDISPLAY\" >(\"Posteingang\" \"mail-folder-inbox\")</attribute>  \
 </collection>");

const QByteArray collection2(
"<collection rid=\"c11\" name=\"Inbox\" content=\"inode/directory,message/rfc822\">           \
      <attribute type=\"ENTITYDISPLAY\" >(\"Posteingang\" \"mail-folder-inbox\")</attribute>  \
      <collection rid=\"c111\" name=\"KDE PIM\" content=\"inode/directory,message/rfc822\">   \
      </collection>            								      \
      <collection rid=\"c112\" name=\"Akonadi\" content=\"inode/directory,message/rfc822\">   \
        <attribute type=\"AccessRights\">wcW</attribute> 		                      \
      </collection>									      \
    </collection>									      \
");

void CollectionTest::testCollection()
{
  QDomDocument mDocument;
  AkonadiXML test;

  mDocument.setContent(collection1, true, 0);

  Collection::List colist = test.buildCollectionTree(mDocument.documentElement());

//  QCOMPARE(colist.count(), 1);
   
}
