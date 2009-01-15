#include "testmaildir.h"
#include "maildirimport.h"
#include "maildirfetchallheaders.h"
#include "maildir20percentread.h"
#include "maildirfetchunreadheaders.h"

TestMailDir::TestMailDir(const QString &dir)
{
  addTest( new MailDirImport(dir));
  addTest( new MailDirFetchAllHeaders());
  addTest( new MailDir20PercentAsRead());
  addTest( new MailDirFetchUnreadHeaders());
}   
