#ifndef MAILDIRFETCHUNREADHEADERS_H
#define MAILDIRFETCHUNREADHEADERS_H

#include "maildir.h"

class MailDirFetchUnreadHeaders: public MailDir {

  public:
    MailDirFetchUnreadHeaders();
    void runTest();
};
#endif
