#ifndef VCARD_IMPORT_H
#define VCARD_IMPORT_H

#include "vcard.h"

class VCardImport : public VCard {
  public:
    VCardImport(const QString &dir);
    void runTest();
};

#endif
