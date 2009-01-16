#ifndef MAILDIRIMPORT_H
#define MAILDIRIMPORT_H

#include "maildir.h"

class MailDirImport: public MailDir 
{

  public:
    MailDirImport(const QString &dir);
    void runTest();
};
#endif
