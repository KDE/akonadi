/*
    SPDX-FileCopyrightText: 2006 Volker Krause <vkrause@kde.org>
    SPDX-FileCopyrightText: 2007 Robert Zwerus <arzie@dds.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "itemdumper.h"
#include "collectionpathresolver.h"

#include "item.h"

#include <QDebug>
#include <QFile>
#include <QApplication>

#include <KLocalizedString>

#include <KAboutData>
#include <QCommandLineParser>
#include <QCommandLineOption>

#include "transactionjobs.h"
#include "itemcreatejob.h"

#define GLOBAL_TRANSACTION 1

using namespace Akonadi;

ItemDumper::ItemDumper(const QString &path, const QString &filename, const QString &mimetype, int count)
    : mJobCount(0)
{
    auto *resolver = new CollectionPathResolver(path, this);
    Q_ASSERT(resolver->exec());
    const Collection collection = Collection(resolver->collection());

    QFile f(filename);
    Q_ASSERT(f.open(QIODevice::ReadOnly));
    QByteArray data = f.readAll();
    f.close();
    Item item;
    item.setMimeType(mimetype);
    item.setPayloadFromData(data);
    mTime.start();
#ifdef GLOBAL_TRANSACTION
    auto *begin = new TransactionBeginJob(this);
    connect(begin, &TransactionBeginJob::result, this, &ItemDumper::done);
    ++mJobCount;
#endif
    for (int i = 0; i < count; ++i) {
        ++mJobCount;
        auto *job = new ItemCreateJob(item, collection, this);
        connect(job, &ItemCreateJob::result, this, &ItemDumper::done);
    }
#ifdef GLOBAL_TRANSACTION
    auto *commit = new TransactionCommitJob(this);
    connect(commit, &TransactionCommitJob::result, this, &ItemDumper::done);
    ++mJobCount;
#endif
}

void ItemDumper::done(KJob *job)
{
    --mJobCount;
    if (job->error()) {
        qWarning() << "Error while creating item: " << job->errorString();
    }
    if (mJobCount <= 0) {
        qDebug() << "Time:" << mTime.elapsed() << "ms";
        qApp->quit();
    }
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    KAboutData aboutData(QStringLiteral("test"),
                         i18n("Test Application"),
                         QStringLiteral("1.0"));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("path"), i18n("IMAP destination path"), QStringLiteral("argument")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("mimetype"), i18n("Source mimetype"), QStringLiteral("argument"), QStringLiteral("application/octet-stream")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("file"), i18n("Source file"), QStringLiteral("argument")));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("count"), i18n("Number of times this file is added"), QStringLiteral("argument"), QStringLiteral("1")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QString path = parser.value(QStringLiteral("path"));
    QString mimetype = parser.value(QStringLiteral("mimetype"));
    QString file = parser.value(QStringLiteral("file"));
    int count = qMax(1, parser.value(QStringLiteral("count")).toInt());
    ItemDumper d(path, file, mimetype, count);
    return app.exec();
}
