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
#include <xml/xmlreader.h>
#include <xml/xmlwriter.h>

#include <akonadi/entitydisplayattribute.h>

#include <qtest_kde.h>
#include <kdebug.h>

QTEST_KDEMAIN(CollectionTest, NoGUI)

QByteArray collection1(
"<test>\n"
" <collection content=\"inode/directory,message/rfc822\" rid=\"c11\" name=\"Inbox\" >\n"
"  <attribute type=\"ENTITYDISPLAY\" >(\"Posteingang\" \"mail-folder-inbox\")</attribute>\n"
" </collection>\n"
"</test>\n");

QByteArray collection2(
"<test> \
  <collection rid=\"c11\" name=\"Inbox\" content=\"inode/directory,message/rfc822\">           \
      <attribute type=\"ENTITYDISPLAY\" >(\"Posteingang\" \"mail-folder-inbox\")</attribute>  \
      <collection rid=\"c111\" name=\"KDE PIM\" content=\"inode/directory,message/rfc822\">   \
      </collection>            								      \
      <collection rid=\"c112\" name=\"Akonadi\" content=\"inode/directory,message/rfc822\">   \
        <attribute type=\"AccessRights\">wcW</attribute> 		                      \
      </collection>									      \
    </collection>									      \
<test>");

void CollectionTest::testBuildCollection()
{
  QDomDocument mDocument;

  mDocument.setContent(collection1, true, 0);
  Collection::List colist = XmlReader::readCollections( mDocument.documentElement());

  QStringList mimeType;

  mimeType << "inode/directory" << "message/rfc822";
  QCOMPARE(colist.size(), 1);
  verifyCollection(colist, 0, "c11", "Inbox", mimeType);

  mDocument.setContent(collection2, true, 0);
  colist = XmlReader::readCollections( mDocument.documentElement());

  QCOMPARE(colist.size(), 3);
  verifyCollection(colist, 0, "c11", "Inbox", mimeType);
  verifyCollection(colist, 1, "c111", "KDE PIM", mimeType);
  verifyCollection(colist, 2, "c112", "Akonadi", mimeType);

  QVERIFY( colist.at( 0 ).hasAttribute<EntityDisplayAttribute>() );
  EntityDisplayAttribute *attr = colist.at( 0 ).attribute<EntityDisplayAttribute>();
  QCOMPARE( attr->displayName(), QString( "Posteingang" ) );
}

void CollectionTest::serializeCollection()
{
  Collection c;
  c.setRemoteId( "c11" );
  c.setName( "Inbox" );
  c.setContentMimeTypes( QStringList() << Collection::mimeType() << "message/rfc822" );
  c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( "Posteingang" );
  c.attribute<EntityDisplayAttribute>()->setIconName( "mail-folder-inbox" );

  QDomDocument doc;
  QDomElement root = doc.createElement( "test" );
  doc.appendChild( root );
  XmlWriter::writeCollection( c, root );

  QCOMPARE( doc.toString(), QString::fromUtf8( collection1 ) );
}

void CollectionTest::verifyCollection(Collection::List colist, int listPosition,QString remoteId, 
    QString name, QStringList mimeType)
{
  QVERIFY(colist.at(listPosition).name() == name);
  QVERIFY(colist.at(listPosition).remoteId() == remoteId);
  QVERIFY(colist.at(listPosition).contentMimeTypes() == mimeType);
}
