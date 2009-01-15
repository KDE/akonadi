#ifndef MAILDIRREMOVEREADMESSAGES_H
#define MAILDIRREMOVEREADMESSAGES_H

#include "maildir.h"

class MailDirRemoveReadMessages: public MailDir {

  public:
    MailDirRemoveReadMessages();
    void runTest();
};
#endif
