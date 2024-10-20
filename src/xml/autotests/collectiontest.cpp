/*
    SPDX-FileCopyrightText: Igor Trindade Oliveira <igor_trindade@yahoo.com.br>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "collectiontest.h"
#include "xmlreader.h"
#include "xmlwriter.h"

#include "entitydisplayattribute.h"

#include <QStringList>
#include <QTest>
using namespace Akonadi;

QTEST_MAIN(CollectionTest)

// NOTE: XML element attributes are stored in QHash, which means that there are
// always in random order when converting to string. This test has QT_HASH_SEED
// always set to 0, but it appears that it has different effect on my computer
// and on Jenkins. This order of attributes is the order that passes on Jenkis,
// so if it fails for you locally because of different order of arguments,
// please make sure that your fix won't break the test on Jenkins.

QByteArray collection1(R"r(<test>
 <collection content="inode/directory,message/rfc822" name="Inbox" rid="c11">
  <attribute type="ENTITYDISPLAY">("Posteingang" "mail-folder-inbox" "" ())</attribute>
 </collection>
</test>
)r");

QByteArray collection2(R"r(<test>
 <collection  content="inode/directory,message/rfc822" name="Inbox" rid="c11">
  <attribute type="ENTITYDISPLAY" >("Posteingang" "mail-folder-inbox" false)</attribute>
   <collection rid="c111" name="KDE PIM" content="inode/directory,message/rfc822">
   </collection>
   <collection rid="c112" name="Akonadi" content="inode/directory,message/rfc822">
    <attribute type="AccessRights">wcW</attribute>
   </collection>
 </collection>
<test>
)r");

void CollectionTest::testBuildCollection()
{
    QDomDocument mDocument;

    QDomDocument::ParseResult parseResult = mDocument.setContent(collection1, QDomDocument::ParseOption::UseNamespaceProcessing);
    if (!parseResult) {
        qDebug() << "Unable to load document.Parse error in line " << parseResult.errorLine << ", col " << parseResult.errorColumn << ": "
                 << qPrintable(parseResult.errorMessage);
    }

    Collection::List colist = XmlReader::readCollections(mDocument.documentElement());

    const QStringList mimeType{QStringLiteral("inode/directory"), QStringLiteral("message/rfc822")};
    QCOMPARE(colist.size(), 1);
    verifyCollection(colist, 0, QStringLiteral("c11"), QStringLiteral("Inbox"), mimeType);

    parseResult = mDocument.setContent(collection2, QDomDocument::ParseOption::UseNamespaceProcessing);
    if (!parseResult) {
        qDebug() << "Unable to load document.Parse error in line " << parseResult.errorLine << ", col " << parseResult.errorColumn << ": "
                 << qPrintable(parseResult.errorMessage);
    }
    colist = XmlReader::readCollections(mDocument.documentElement());

    QCOMPARE(colist.size(), 3);
    verifyCollection(colist, 0, QStringLiteral("c11"), QStringLiteral("Inbox"), mimeType);
    verifyCollection(colist, 1, QStringLiteral("c111"), QStringLiteral("KDE PIM"), mimeType);
    verifyCollection(colist, 2, QStringLiteral("c112"), QStringLiteral("Akonadi"), mimeType);

    QVERIFY(colist.at(0).hasAttribute<EntityDisplayAttribute>());
    const auto attr = colist.at(0).attribute<EntityDisplayAttribute>();
    QCOMPARE(attr->displayName(), QStringLiteral("Posteingang"));
}

void CollectionTest::serializeCollection()
{
    Collection c;
    c.setRemoteId(QStringLiteral("c11"));
    c.setName(QStringLiteral("Inbox"));
    c.setContentMimeTypes(QStringList() << Collection::mimeType() << QStringLiteral("message/rfc822"));
    c.attribute<EntityDisplayAttribute>(Collection::AddIfMissing)->setDisplayName(QStringLiteral("Posteingang"));
    c.attribute<EntityDisplayAttribute>()->setIconName(QStringLiteral("mail-folder-inbox"));

    QDomDocument doc;
    QDomElement root = doc.createElement(QStringLiteral("test"));
    doc.appendChild(root);
    XmlWriter::writeCollection(c, root);

    QCOMPARE(doc.toString(), QString::fromUtf8(collection1));
}

void CollectionTest::verifyCollection(const Collection::List &colist,
                                      int listPosition,
                                      const QString &remoteId,
                                      const QString &name,
                                      const QStringList &mimeType)
{
    QVERIFY(colist.at(listPosition).name() == name);
    QVERIFY(colist.at(listPosition).remoteId() == remoteId);
    QVERIFY(colist.at(listPosition).contentMimeTypes() == mimeType);
}

#include "moc_collectiontest.cpp"
