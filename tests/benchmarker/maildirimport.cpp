#include "maildirimport.h"
#include "maildir.h"
#include <QDebug>
#include <QTest>

#define WAIT_TIME 100

MailDirImport::MailDirImport(const QString &dir):MailDir(dir){}

void MailDirImport::runTest() {
  done = false;
  timer.start();
  qDebug() << "  Synchronising resource.";
  currentInstance.synchronize();
  while(!done)
    QTest::qWait( WAIT_TIME );
  outputStats( "import" );
}


