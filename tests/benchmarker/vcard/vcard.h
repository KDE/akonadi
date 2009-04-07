#ifndef VCARD_H
#define VCARD_H

#include <QTest>

#include "../maketest.h"

class VCard : public MakeTest {

  public:
    VCard(const QString &dir);
    VCard();
};

#endif
