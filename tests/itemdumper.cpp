/*
    Copyright (c) 2006 Volker Krause <vkrause@kde.org>
    Copyright (c) 2007 Robert Zwerus <arzie@dds.nl>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#include "itemdumper.h"
#include "collectionpathresolver.h"

#include "item.h"

#include <QDebug>
#include <QFile>
#include <QApplication>

#include <klocale.h>


#include <KAboutData>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "transactionjobs.h"
#include "itemcreatejob.h"

#define GLOBAL_TRANSACTION 1

using namespace Akonadi;

ItemDumper::ItemDumper( const QString &path, const QString &filename, const QString &mimetype, int count ) :
    mJobCount( 0 )
{
  CollectionPathResolver* resolver = new CollectionPathResolver( path, this );
  Q_ASSERT( resolver->exec() );
  const Collection collection = Collection( resolver->collection() );

  QFile f( filename );
  Q_ASSERT( f.open(QIODevice::ReadOnly) );
  QByteArray data = f.readAll();
  f.close();
  Item item;
  item.setMimeType( mimetype );
  item.setPayloadFromData( data );
  mTime.start();
#ifdef GLOBAL_TRANSACTION
  TransactionBeginJob *begin = new TransactionBeginJob( this );
  connect( begin, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
  ++mJobCount;
#endif
  for ( int i = 0; i < count; ++i ) {
    ++mJobCount;
    ItemCreateJob *job = new ItemCreateJob( item, collection, this );
    connect( job, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
  }
#ifdef GLOBAL_TRANSACTION
  TransactionCommitJob *commit = new TransactionCommitJob( this );
  connect( commit, SIGNAL(result(KJob*)), SLOT(done(KJob*)) );
  ++mJobCount;
#endif
}

void ItemDumper::done( KJob * job )
{
  --mJobCount;
  if ( job->error() ) {
    qWarning() << "Error while creating item: " << job->errorString();
  }
  if ( mJobCount <= 0 ) {
    qDebug() << "Time:" << mTime.elapsed() << "ms";
    qApp->quit();
  }
}

int main( int argc, char** argv )
{
    KAboutData aboutData( QStringLiteral("test"),
                          i18n("Test Application"),
                          QLatin1String("1.0"));

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("path"), i18n("IMAP destination path"), QLatin1String("argument")));
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("mimetype"), i18n("Source mimetype"), QLatin1String("argument"), QLatin1String("application/octet-stream")));
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("file"), i18n("Source file"), QLatin1String("argument")));
  parser.addOption(QCommandLineOption(QStringList() << QLatin1String("count"), i18n("Number of times this file is added"), QLatin1String("argument"), QLatin1String("1")));

    //PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

  QString path = parser.value( QLatin1String("path") );
  QString mimetype = parser.value( QLatin1String("mimetype") );
  QString file = parser.value( QLatin1String("file") );
  int count = qMax( 1, parser.value( QLatin1String("count") ).toInt() );
  ItemDumper d( path, file, mimetype, count );
  return app.exec();
}

