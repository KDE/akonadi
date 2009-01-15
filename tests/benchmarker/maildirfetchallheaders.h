#ifndef MAILDIRFETCHALLHEADERS_H
#define MAILDIRIMPORT_H

#include "maildir.h"

class MailDirFetchAllHeaders: public MailDir {

  public:
    MailDirFetchAllHeaders();
    void runTest();
};
#endif
