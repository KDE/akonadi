/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

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

#include "akapplication.h"
#include "akdebug.h"

#include <private/instance_p.h>
#include <akonadi_version.h>

#include <QDBusConnection>
#include <QTimer>
#include <QCommandLineParser>
#include <QCoreApplication>

#include <iostream>

AkApplicationBase *AkApplicationBase::sInstance = 0;

AkApplicationBase::AkApplicationBase(int &argc, char **argv, const QLoggingCategory &loggingCategory)
    : QObject(0)
    , mArgc(argc)
    , mArgv(argv)
    , mLoggingCategory(loggingCategory)
{
    Q_ASSERT(!sInstance);
    sInstance = this;

    QCoreApplication::setApplicationName(QStringLiteral("Akonadi"));
    QCoreApplication::setApplicationVersion(QStringLiteral(AKONADI_VERSION_STRING));
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
    akInit(QString::fromLatin1(mArgv[0]));

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
