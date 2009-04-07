#include "testmaildir.h"
#include "maildir/maildirimport.h"
#include "maildir/maildirfetchallheaders.h"
#include "maildir/maildir20percentread.h"
#include "maildir/maildirfetchunreadheaders.h"

TestMailDir::TestMailDir(const QString &dir)
{
  addTest( new MailDirImport(dir));
  addTest( new MailDirFetchAllHeaders());
  addTest( new MailDir20PercentAsRead());
  addTest( new MailDirFetchUnreadHeaders());
}
