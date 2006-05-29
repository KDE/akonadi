#include <QtTest/QtTest>

#include "handler.h"
#include "handler/list.h"

using namespace Akonadi;

class TestHandler: public QObject {
  Q_OBJECT
private slots:

  void testInit()
  {
  }

  void testList()
  {
      List* l = new List();
  }

};

QTEST_MAIN(TestHandler)

#include "main.moc"
