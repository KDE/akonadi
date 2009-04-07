#include "testvcard.h"
#include "vcard/vcardimport.h"

TestVCard::TestVCard(const QString &dir)
{
  addTest( new VCardImport(dir));
}
