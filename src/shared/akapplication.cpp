/*
    SPDX-FileCopyrightText: 2008 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "akapplication.h"
#include "akdebug.h"
#include "akremotelog.h"

#include <private/instance_p.h>
#include <akonadifull-version.h>

#include <QTimer>
#include <QCoreApplication>

#include <iostream>

AkApplicationBase *AkApplicationBase::sInstance = nullptr;

AkApplicationBase::AkApplicationBase(std::unique_ptr<QCoreApplication> app, const QLoggingCategory &loggingCategory)
    : QObject(nullptr)
    , mApp(std::move(app))
    , mLoggingCategory(loggingCategory)
{
    Q_ASSERT(!sInstance);
    sInstance = this;

    QCoreApplication::setApplicationName(QStringLiteral("Akonadi"));
    QCoreApplication::setApplicationVersion(QStringLiteral(AKONADI_FULL_VERSION));
    mCmdLineParser.addHelpOption();
    mCmdLineParser.addVersionOption();
}

AkApplicationBase::~AkApplicationBase()
{
}

AkApplicationBase *AkApplicationBase::instance()
{
    Q_ASSERT(sInstance);
    return sInstance;
}

void AkApplicationBase::init()
{
    akInit(mApp->applicationName());
    akInitRemoteLog();

    if (!QDBusConnection::sessionBus().isConnected()) {
        qFatal("D-Bus session bus is not available!");
    }

    // there doesn't seem to be a signal to indicate that the session bus went down, so lets use polling for now
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &AkApplicationBase::pollSessionBus);
    timer->start(10 * 1000);
}

void AkApplicationBase::setDescription(const QString &desc)
{
    mCmdLineParser.setApplicationDescription(desc);
}

void AkApplicationBase::parseCommandLine()
{
    const QCommandLineOption instanceOption(QStringList() << QStringLiteral("instance"),
                                            QStringLiteral("Namespace for starting multiple Akonadi instances in the same user session"),
                                            QStringLiteral("name"));
    mCmdLineParser.addOption(instanceOption);
    const QCommandLineOption verboseOption(QStringLiteral("verbose"),
                                           QStringLiteral("Make Akonadi very chatty"));
    mCmdLineParser.addOption(verboseOption);

    mCmdLineParser.process(QCoreApplication::arguments());

    if (mCmdLineParser.isSet(instanceOption)) {
        Akonadi::Instance::setIdentifier(mCmdLineParser.value(instanceOption));
    }
    if (mCmdLineParser.isSet(verboseOption)) {
        akMakeVerbose(mLoggingCategory.categoryName());
    }
}

void AkApplicationBase::pollSessionBus() const
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        qCritical("D-Bus session bus went down - quitting");
        mApp->quit();
    }
}

void AkApplicationBase::addCommandLineOptions(const QCommandLineOption &option)
{
    mCmdLineParser.addOption(option);
}

void AkApplicationBase::addPositionalCommandLineOption(const QString &name, const QString &description, const QString &syntax)
{
    mCmdLineParser.addPositionalArgument(name, description, syntax);
}

void AkApplicationBase::printUsage() const
{
    std::cout << qPrintable(mCmdLineParser.helpText()) << std::endl;
}

int AkApplicationBase::exec()
{
    return mApp->exec();
}

QString akGetEnv(const char *name, const QString &defaultValue)
{
    const QString v = QString::fromLocal8Bit(qgetenv(name));
    return !v.isEmpty() ? v : defaultValue;
}
