#include "vcard.h"

VCard::VCard(const QString &dir) : MakeTest()
{
  createAgent("akonadi_vcarddir_resource");
  configureDBusIface("VCard", dir);
}

VCard::VCard() : MakeTest(){}
