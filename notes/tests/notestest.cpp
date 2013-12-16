/*
    Copyright (c) 2012 Christian Mollekopf <mollekopf@kolabsys.com>

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

#include "noteutils.h"

#include <QTest>
#include <QHash>
#include <QDebug>

#include <KDateTime>
#include <kmime/kmime_message.h>

using namespace Akonadi::NoteUtils;
class NotesTest : public QObject
{
  Q_OBJECT
  private Q_SLOTS:

    void testSerializeAndParse()
    {
      NoteMessageWrapper note;
      note.setTitle("title");
      note.setText("title");
      note.setUid("uid");
      note.setClassification(NoteMessageWrapper::Private);
      note.setFrom("from@kde.org");
      note.setCreationDate(KDateTime(QDate(2012,3,3), QTime(3,3,3), KDateTime::UTC));
      note.setLastModifiedDate(KDateTime(QDate(2012,3,3), QTime(4,4,4), KDateTime::UTC));
      Attachment a("testfile2", "mimetype/mime3");
      a.setLabel("label");
      note.attachments() << Attachment(QUrl("file://url/to/file"), "mimetype/mime") << Attachment("testfile", "mimetype/mime2") << a;
      note.custom().insert("key1", "value1");
      note.custom().insert("key2", "value2");
      note.custom().insert("key3", "value3");

      KMime::MessagePtr msg = note.message();
//       qWarning() << msg->encodedContent();

      NoteMessageWrapper result(msg);

      QCOMPARE(result.title(), note.title());
      QCOMPARE(result.text(), note.text());
      QCOMPARE(result.textFormat(), note.textFormat());
      QCOMPARE(result.uid(), note.uid());
      QCOMPARE(result.classification(), note.classification());
      QCOMPARE(result.from(), note.from());
      QCOMPARE(result.creationDate(), note.creationDate());
      QCOMPARE(result.lastModifiedDate(), note.lastModifiedDate());
      QCOMPARE(result.custom(), note.custom());
      QCOMPARE(result.attachments(), note.attachments());

//       qWarning() << result.message()->encodedContent();
    }

    void createIfEmpty()
    {
      NoteMessageWrapper note;
      NoteMessageWrapper result(note.message());
//       qDebug() << result.uid();
      QVERIFY(!result.uid().isEmpty());
      QVERIFY(result.creationDate().isValid());
      QVERIFY(result.lastModifiedDate().isValid());
    }

};

QTEST_MAIN( NotesTest )

#include "notestest.moc"
