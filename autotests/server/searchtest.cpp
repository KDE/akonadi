/*
 * SPDX-FileCopyrightText: 2014 Daniel Vrátil <dvratil@redhat.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "aktest.h"
#include "fakeakonadiserver.h"
#include "handler/searchhelper.h"

#include <entities.h>

#include <QTest>

using namespace Akonadi::Server;

Q_DECLARE_METATYPE(QList<qint64>)
Q_DECLARE_METATYPE(QList<QString>)

class SearchTest : public QObject
{
    Q_OBJECT

    FakeAkonadiServer mAkonadi;

public:
    SearchTest()
        : QObject()
    {
        mAkonadi.setPopulateDb(false);
        mAkonadi.init();
    }

    Collection createCollection(const Resource &res, const QString &name, const Collection &parent, const QStringList &mimetypes)
    {
        Collection col;
        col.setName(name);
        col.setResource(res);
        col.setParentId(parent.isValid() ? parent.id() : 0);
        col.insert();
        for (const QString &mimeType : mimetypes) {
            MimeType mt = MimeType::retrieveByName(mimeType);
            if (!mt.isValid()) {
                mt = MimeType(mimeType);
                mt.insert();
            }
            col.addMimeType(mt);
        }
        return col;
    }

private Q_SLOTS:
    void testSearchHelperCollectionListing_data()
    {
        /*
        Fake Resource
          |- Col 1 (inode/directory)
          |  |- Col 2 (inode/direcotry, application/octet-stream)
          |  |  |- Col 3(application/octet-stream)
          |  |- Col 4 (text/plain)
          |- Col 5 (inode/directory, text/plain)
             |- Col 6 (inode/directory, application/octet-stream)
             |- Col 7 (inode/directory, text/plain)
                 |- Col 8 (inode/directory, application/octet-stream)
                    |- Col 9 (unique/mime-type)
        */

        Resource res(QStringLiteral("Test Resource"), false);
        res.insert();

        Collection col1 = createCollection(res, QStringLiteral("Col 1"), Collection(), QStringList() << QStringLiteral("inode/directory"));
        Collection col2 = createCollection(res,
                                           QStringLiteral("Col 2"),
                                           col1,
                                           QStringList() << QStringLiteral("inode/directory") << QStringLiteral("application/octet-stream"));
        Collection col3 = createCollection(res, QStringLiteral("Col 3"), col2, QStringList() << QStringLiteral("application/octet-stream"));
        Collection col4 = createCollection(res, QStringLiteral("Col 4"), col2, QStringList() << QStringLiteral("text/plain"));
        Collection col5 =
            createCollection(res, QStringLiteral("Col 5"), Collection(), QStringList() << QStringLiteral("inode/directory") << QStringLiteral("text/plain"));
        Collection col6 = createCollection(res,
                                           QStringLiteral("Col 6"),
                                           col5,
                                           QStringList() << QStringLiteral("inode/directory") << QStringLiteral("application/octet-stream"));
        Collection col7 =
            createCollection(res, QStringLiteral("Col 7"), col5, QStringList() << QStringLiteral("inode/directory") << QStringLiteral("text/plain"));
        Collection col8 = createCollection(res,
                                           QStringLiteral("Col 8"),
                                           col7,
                                           QStringList() << QStringLiteral("text/directory") << QStringLiteral("application/octet-stream"));
        Collection col9 = createCollection(res, QStringLiteral("Col 9"), col8, QStringList() << QStringLiteral("unique/mime-type"));

        QTest::addColumn<QVector<qint64>>("ancestors");
        QTest::addColumn<QStringList>("mimetypes");
        QTest::addColumn<QVector<qint64>>("expectedResults");

        QTest::newRow("") << (QVector<qint64>() << 0) << (QStringList() << QStringLiteral("text/plain"))
                          << (QVector<qint64>() << col4.id() << col5.id() << col7.id());
        QTest::newRow("") << (QVector<qint64>() << 0) << (QStringList() << QStringLiteral("application/octet-stream"))
                          << (QVector<qint64>() << col2.id() << col3.id() << col6.id() << col8.id());
        QTest::newRow("") << (QVector<qint64>() << col1.id()) << (QStringList() << QStringLiteral("text/plain")) << (QVector<qint64>() << col4.id());
        QTest::newRow("") << (QVector<qint64>() << col1.id()) << (QStringList() << QStringLiteral("unique/mime-type")) << QVector<qint64>();
        QTest::newRow("") << (QVector<qint64>() << col2.id() << col7.id()) << (QStringList() << QStringLiteral("application/octet-stream"))
                          << (QVector<qint64>() << col3.id() << col8.id());
    }

    void testSearchHelperCollectionListing()
    {
        QFETCH(QVector<qint64>, ancestors);
        QFETCH(QStringList, mimetypes);
        QFETCH(QVector<qint64>, expectedResults);

        QVector<qint64> results = SearchHelper::matchSubcollectionsByMimeType(ancestors, mimetypes);

        std::sort(expectedResults.begin(), expectedResults.end());
        std::sort(results.begin(), results.end());

        QCOMPARE(results.size(), expectedResults.size());
        QCOMPARE(results, expectedResults);
    }
};

AKTEST_FAKESERVER_MAIN(SearchTest)

#include "searchtest.moc"
