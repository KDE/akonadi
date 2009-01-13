#include "testmaildir.h"
#include <kcmdlineargs.h>
#include <kapplication.h>

int main(int argc, char *argv[])
{
  KCmdLineArgs::init( argc, argv, "benchmarker", 0, ki18n("Benchmarker") ,"1.0" ,ki18n("benchmark application") );

  KCmdLineOptions options;
  options.add("maildir <argument>", ki18n("Path to maildir to be used as data source"));
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;
  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
  QString maildir = args->getOption( "maildir" );

  TestMailDir *mailDirTest = new TestMailDir(maildir);
  mailDirTest->runBenchMarker();

  return app.exec();
}
