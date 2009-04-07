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
