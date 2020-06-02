/*
 * Copyright (C) 2015  Daniel Vrátil <dvratil@redhat.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <private/externalpartstorage_p.h>
#include <private/standarddirs_p.h>
#include <shared/aktest.h>

#include <QObject>
#include <QFile>
#include <QDir>

using namespace Akonadi;

class ExternalPartStorageTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testResolveAbsolutePath_data();
    void testResolveAbsolutePath();

    void testUpdateFileNameRevision_data();
    void testUpdateFileNameRevision();

    void testNameForPartId_data();
    void testNameForPartId();

    void testPartCreate();
    void testPartUpdate();
    void testPartDelete();
    void testPartCreateTrxRollback();
    void testPartUpdateTrxRollback();
    void testPartDeleteTrxRollback();
    void testPartCreateTrxCommit();
    void testPartUpdateTrxCommit();
    void testPartDeleteTrxCommit();
};

void ExternalPartStorageTest::testResolveAbsolutePath_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("expectedLevelCache");
    QTest::addColumn<bool>("isAbsolute");

    QTest::newRow("1_r0") << QStringLiteral("1_r0") << QStringLiteral("01") << false;
    QTest::newRow("23_r0") << QStringLiteral("23_r0") << QStringLiteral("23") << false;
    QTest::newRow("567_r0") << QStringLiteral("567_r0") << QStringLiteral("67") << false;
    QTest::newRow("123456_r0") << QStringLiteral("123456_r0") << QStringLiteral("56") << false;
    QTest::newRow("absolute path") << QStringLiteral("/tmp/akonadi/file_db_data/99_r3") << QString() << true;
}


void ExternalPartStorageTest::testResolveAbsolutePath()
{
    QFETCH(QString, filename);
    QFETCH(QString, expectedLevelCache);
    QFETCH(bool, isAbsolute);

    // Calculate the new levelled-cache hierarchy path
    const QString expectedBasePath = StandardDirs::saveDir("data", QStringLiteral("file_db_data")) + QDir::separator();
    const QString expectedPath = expectedBasePath + expectedLevelCache + QDir::separator();
    const QString expectedFilePath = expectedPath + filename;

    // File does not exist, will return path for the new levelled hierarchy
    // (unless the path is absolute, then it returns the absolute path)
    bool exists = false;
    QString path = ExternalPartStorage::resolveAbsolutePath(filename, &exists);
    QVERIFY(!exists);
    if (isAbsolute) {
        // Absolute path is no further resolved
        QCOMPARE(path, filename);
        return;
    }
    QCOMPARE(path, expectedFilePath);

    // Create the file in the old flat hierarchy
    QFile(expectedBasePath + filename).open(QIODevice::WriteOnly);
    QCOMPARE(ExternalPartStorage::resolveAbsolutePath(filename, &exists), QString(expectedBasePath + filename));
    QVERIFY(exists);
    QVERIFY(QFile::remove(expectedBasePath + filename));

    // Create the file in the new hierarchy - will return the same as the first
    QDir().mkpath(expectedPath);
    QFile(expectedFilePath).open(QIODevice::WriteOnly);
    exists = false;
    // Check that this time we got the new path and exists flag is correctly set
    path = ExternalPartStorage::resolveAbsolutePath(filename, &exists);
    QCOMPARE(path, expectedFilePath);
    QVERIFY(exists);

    // Clean up
    QVERIFY(QFile::remove(path));
}

void ExternalPartStorageTest::testUpdateFileNameRevision_data()
{
    QTest::addColumn<QByteArray>("name");
    QTest::addColumn<QByteArray>("expectedName");

    QTest::newRow("no revision") << QByteArray("1234") << QByteArray("1234_r0");
    QTest::newRow("r0") << QByteArray("1234_r0") << QByteArray("1234_r1");
    QTest::newRow("r12") << QByteArray("1234_r12") << QByteArray("1234_r13");
    QTest::newRow("r123456") << QByteArray("1234_r123456") << QByteArray("1234_r123457");
}

void ExternalPartStorageTest::testUpdateFileNameRevision()
{
    QFETCH(QByteArray, name);
    QFETCH(QByteArray, expectedName);

    const QByteArray newName = ExternalPartStorage::updateFileNameRevision(name);
    QCOMPARE(newName, expectedName);
}

void ExternalPartStorageTest::testNameForPartId_data()
{
    QTest::addColumn<qint64>("id");
    QTest::addColumn<QByteArray>("expectedName");

    QTest::newRow("0") << 0LL << QByteArray("0_r0");
    QTest::newRow("12") << 12LL << QByteArray("12_r0");
    QTest::newRow("9876543") << 9876543LL << QByteArray("9876543_r0");
}


void ExternalPartStorageTest::testNameForPartId()
{
    QFETCH(qint64, id);
    QFETCH(QByteArray, expectedName);

    const QByteArray name = ExternalPartStorage::nameForPartId(id);
    QCOMPARE(name, expectedName);
}

void ExternalPartStorageTest::testPartCreate()
{
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 1, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));
    QFile f(filePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("blabla"));
    f.close();
    QVERIFY(f.remove());
}

void ExternalPartStorageTest::testPartUpdate()
{
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 10, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));

    QByteArray newfilename;
    QVERIFY(ExternalPartStorage::self()->updatePartFile("newdata", filename, newfilename));
    QCOMPARE(ExternalPartStorage::updateFileNameRevision(filename), newfilename);
    const QString newFilePath = ExternalPartStorage::resolveAbsolutePath(newfilename);
    QVERIFY(!QFile::exists(filePath));
    QVERIFY(QFile::exists(newFilePath));

    QFile f(newFilePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("newdata"));
    f.close();
    QVERIFY(f.remove());
}

void ExternalPartStorageTest::testPartDelete()
{
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 2, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));
    QVERIFY(ExternalPartStorage::self()->removePartFile(filePath));
    QVERIFY(!QFile::exists(filePath));
}


void ExternalPartStorageTest::testPartCreateTrxRollback()
{
    ExternalPartStorageTransaction trx;
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 3, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));
    QFile f(filePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("blabla"));
    f.close();
    QVERIFY(trx.rollback());
    QVERIFY(!QFile::exists(filePath));
}

void ExternalPartStorageTest::testPartUpdateTrxRollback()
{
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 10, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));

    ExternalPartStorageTransaction trx;

    QByteArray newfilename;
    QVERIFY(ExternalPartStorage::self()->updatePartFile("newdata", filename, newfilename));
    QCOMPARE(ExternalPartStorage::updateFileNameRevision(filename), newfilename);
    const QString newFilePath = ExternalPartStorage::resolveAbsolutePath(newfilename);
    QVERIFY(QFile::exists(filePath));
    QVERIFY(QFile::exists(newFilePath));

    QFile f(newFilePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("newdata"));
    f.close();

    trx.rollback();
    QVERIFY(QFile::exists(filePath));
    QVERIFY(!QFile::exists(newFilePath));

    QFile f2(filePath);
    QVERIFY(f2.open(QIODevice::ReadOnly));
    QCOMPARE(f2.readAll(), QByteArray("blabla"));
    f2.close();
    QVERIFY(f2.remove());
}

void ExternalPartStorageTest::testPartDeleteTrxRollback()
{
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 4, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));

    ExternalPartStorageTransaction trx;
    QVERIFY(ExternalPartStorage::self()->removePartFile(filePath));
    QVERIFY(QFile::exists(filePath));
    QVERIFY(trx.rollback());
    QVERIFY(QFile::exists(filePath));

    QFile::remove(filePath);
}

void ExternalPartStorageTest::testPartCreateTrxCommit()
{
    ExternalPartStorageTransaction trx;
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 6, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));
    QVERIFY(trx.commit());
    QVERIFY(QFile::exists(filePath));

    QFile f(filePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("blabla"));
    f.close();
    QVERIFY(f.remove());
}

void ExternalPartStorageTest::testPartUpdateTrxCommit()
{
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 10, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));

    ExternalPartStorageTransaction trx;

    QByteArray newfilename;
    QVERIFY(ExternalPartStorage::self()->updatePartFile("newdata", filename, newfilename));
    QCOMPARE(ExternalPartStorage::updateFileNameRevision(filename), newfilename);
    const QString newFilePath = ExternalPartStorage::resolveAbsolutePath(newfilename);
    QVERIFY(QFile::exists(filePath));
    QVERIFY(QFile::exists(newFilePath));

    QFile f(newFilePath);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QCOMPARE(f.readAll(), QByteArray("newdata"));
    f.close();

    trx.commit();
    QVERIFY(!QFile::exists(filePath));
    QVERIFY(QFile::exists(newFilePath));
    QVERIFY(QFile::remove(newFilePath));
}

void ExternalPartStorageTest::testPartDeleteTrxCommit()
{
    QByteArray filename;
    QVERIFY(ExternalPartStorage::self()->createPartFile("blabla", 7, filename));
    const QString filePath = ExternalPartStorage::resolveAbsolutePath(filename);
    QVERIFY(QFile::exists(filePath));

    ExternalPartStorageTransaction trx;
    QVERIFY(ExternalPartStorage::self()->removePartFile(filePath));
    QVERIFY(QFile::exists(filePath));
    QVERIFY(trx.commit());
    QVERIFY(!QFile::exists(filePath));
}




AKTEST_MAIN(ExternalPartStorageTest)

#include "externalpartstoragetest.moc"
