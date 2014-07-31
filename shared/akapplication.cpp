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
#include <config-akonadi.h>

#include <QDBusConnection>
#include <QTimer>
#include <QtCore/QCoreApplication>

#include <iostream>

namespace po = boost::program_options;

AkApplication *AkApplication::sInstance = 0;

AkApplication::AkApplication(int &argc, char **argv)
    : QObject(0)
    , mArgc(argc)
    , mArgv(argv)
{
    Q_ASSERT(!sInstance);
    sInstance = this;
}

AkApplication::~AkApplication()
{
}

AkApplication *AkApplication::instance()
{
    Q_ASSERT(sInstance);
    return sInstance;
}

void AkApplication::init()
{
    akInit(QString::fromLatin1(mArgv[0]));

    if (!QDBusConnection::sessionBus().isConnected()) {
        akFatal() << "D-Bus session bus is not available!";
    }

    // there doesn't seem to be a signal to indicate that the session bus went down, so lets use polling for now
    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(pollSessionBus()));
    timer->start(10 * 1000);
}

void AkApplication::parseCommandLine()
{
    try {
        po::options_description generalOptions("General options");
        generalOptions.add_options()
        ("help,h", "show this help message")
        ("version", "show version information");
        mCmdLineOptions.add(generalOptions);

        po::options_description miOptions("Multi-instance options");
        miOptions.add_options()
        ("instance", po::value<std::string>(), "Namespace for starting multiple Akonadi instances in the same user session");
        mCmdLineOptions.add(miOptions);

        po::command_line_parser parser(mArgc, mArgv);
        parser.options(mCmdLineOptions);
        if (mCmdPositionalOptions.max_total_count() > 0) {
            parser.positional(mCmdPositionalOptions);
        }
        po::store(parser.run(), mCmdLineArguments);
        po::notify(mCmdLineArguments);

        if (mCmdLineArguments.count("help")) {
            printUsage();
            ::exit(0);
        }

        if (mCmdLineArguments.count("version")) {
            std::cout << "Akonadi " << AKONADI_VERSION_STRING << std::endl;
            ::exit(0);
        }

        if (mCmdLineArguments.count("instance")) {
            mInstanceId = QString::fromStdString(mCmdLineArguments["instance"].as<std::string>());
        }
    } catch (std::exception &e) {
        std::cerr << "Failed to parse command line arguments: " << e.what() << std::endl;
        std::cerr << "Run '" << mArgv[0] << " --help' to obtain a list of valid command line arguments." << std::endl;
        ::exit(1);
    }
}

void AkApplication::pollSessionBus() const
{
    if (!QDBusConnection::sessionBus().isConnected()) {
        akError() << "D-Bus session bus went down - quitting";
        mApp->quit();
    }
}

void AkApplication::addCommandLineOptions(const boost::program_options::options_description &desc)
{
    mCmdLineOptions.add(desc);
}

void AkApplication::addPositionalCommandLineOption(const char *option, int count)
{
    mCmdPositionalOptions.add(option, count);
}

void AkApplication::printUsage() const
{
    if (!mDescription.isEmpty()) {
        std::cout << qPrintable(mDescription) << std::endl;
    }
    std::cout << mCmdLineOptions << std::endl;
}

QString AkApplication::instanceIdentifier()
{
    Q_ASSERT(sInstance);
    return sInstance->mInstanceId;
}

bool AkApplication::hasInstanceIdentifier()
{
    return !instanceIdentifier().isEmpty();
}

int AkApplication::exec()
{
    return mApp->exec();
}

void AkApplication::setInstanceIdentifier(const QString &instanceId)
{
    Q_ASSERT(sInstance);
    sInstance->mInstanceId = instanceId;
}
