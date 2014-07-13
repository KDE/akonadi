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
#include <xmlreader.h>
#include <xmlwriter.h>

#include <entitydisplayattribute.h>

#include <qtest.h>
#include <qdebug.h>
using namespace Akonadi;
QTEST_MAIN(CollectionTest)

QByteArray collection1(
"<test>\n"
" <collection content=\"inode/directory,message/rfc822\" rid=\"c11\" name=\"Inbox\">\n"
"  <attribute type=\"ENTITYDISPLAY\">(\"Posteingang\" \"mail-folder-inbox\" \"\" ())</attribute>\n"
" </collection>\n"
"</test>\n");

QByteArray collection2(
"<test> \
  <collection rid=\"c11\" name=\"Inbox\" content=\"inode/directory,message/rfc822\">           \
      <attribute type=\"ENTITYDISPLAY\" >(\"Posteingang\" \"mail-folder-inbox\" false)</attribute>  \
      <collection rid=\"c111\" name=\"KDE PIM\" content=\"inode/directory,message/rfc822\">   \
      </collection>                                                                                  \
      <collection rid=\"c112\" name=\"Akonadi\" content=\"inode/directory,message/rfc822\">   \
        <attribute type=\"AccessRights\">wcW</attribute>                                       \
      </collection>                                                                              \
    </collection>                                                                              \
<test>");

void CollectionTest::testBuildCollection()
{
  QDomDocument mDocument;

  mDocument.setContent( collection1, true, 0 );
  Collection::List colist = XmlReader::readCollections( mDocument.documentElement() );

  QStringList mimeType;

  mimeType << QLatin1String( "inode/directory" ) << QLatin1String( "message/rfc822" );
  QCOMPARE( colist.size(), 1 );
  verifyCollection( colist, 0, QLatin1String( "c11" ), QLatin1String( "Inbox" ), mimeType );

  mDocument.setContent( collection2, true, 0 );
  colist = XmlReader::readCollections( mDocument.documentElement() );

  QCOMPARE( colist.size(), 3 );
  verifyCollection( colist, 0, QLatin1String( "c11" ), QLatin1String( "Inbox" ), mimeType );
  verifyCollection( colist, 1, QLatin1String( "c111" ), QLatin1String( "KDE PIM" ), mimeType );
  verifyCollection( colist, 2, QLatin1String( "c112" ), QLatin1String( "Akonadi" ), mimeType );

  QVERIFY( colist.at( 0 ).hasAttribute<EntityDisplayAttribute>() );
  EntityDisplayAttribute *attr = colist.at( 0 ).attribute<EntityDisplayAttribute>();
  QCOMPARE( attr->displayName(), QString::fromLatin1( "Posteingang" ) );
}

void CollectionTest::serializeCollection()
{
  Collection c;
  c.setRemoteId( QLatin1String( "c11" ) );
  c.setName( QLatin1String( "Inbox" ) );
  c.setContentMimeTypes( QStringList() << Collection::mimeType() << QLatin1String( "message/rfc822" ) );
  c.attribute<EntityDisplayAttribute>( Collection::AddIfMissing )->setDisplayName( QLatin1String( "Posteingang" ) );
  c.attribute<EntityDisplayAttribute>()->setIconName( QLatin1String( "mail-folder-inbox" ) );

  QDomDocument doc;
  QDomElement root = doc.createElement( QLatin1String( "test" ) );
  doc.appendChild( root );
  XmlWriter::writeCollection( c, root );

  QCOMPARE( doc.toString(), QString::fromUtf8( collection1 ) );
}

void CollectionTest::verifyCollection( const Collection::List &colist, int listPosition,
                                       const QString &remoteId, const QString &name,
                                       const QStringList &mimeType )
{
  QVERIFY( colist.at( listPosition ).name() == name );
  QVERIFY( colist.at( listPosition ).remoteId() == remoteId );
  QVERIFY( colist.at( listPosition ).contentMimeTypes() == mimeType );
}
