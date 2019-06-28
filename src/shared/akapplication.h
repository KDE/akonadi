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

#ifndef AKAPPLICATION_H
#define AKAPPLICATION_H

#include <QObject>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QDBusConnection>
#include <QDBusError>

#include <memory>

class QCoreApplication;
class QApplication;
class QGuiApplication;

/**
 * D-Bus session bus monitoring and command line handling.
 */
class AkApplicationBase : public QObject
{
    Q_OBJECT
public:
    ~AkApplicationBase();
    void parseCommandLine();
    void setDescription(const QString &desc);

    void addCommandLineOptions(const QCommandLineOption &option);
    void addPositionalCommandLineOption(const QString &name, const QString &description = QString(), const QString &syntax = QString());
    const QCommandLineParser &commandLineArguments() const
    {
        return mCmdLineParser;
    }

    void printUsage() const;

    /** Returns the AkApplication instance */
    static AkApplicationBase *instance();

    /** Forward to Q[Core]Application for convenience. */
    int exec();

protected:
    AkApplicationBase(std::unique_ptr<QCoreApplication> app, const QLoggingCategory &loggingCategory);
    void init();

    std::unique_ptr<QCoreApplication> mApp;

private Q_SLOTS:
    void pollSessionBus() const;

private:
    QString mInstanceId;
    const QLoggingCategory &mLoggingCategory;
    static AkApplicationBase *sInstance;

    QCommandLineParser mCmdLineParser;
};

template <typename T>
class AkApplicationImpl : public AkApplicationBase
{
public:
    AkApplicationImpl(int &argc, char **argv, const QLoggingCategory &loggingCategory = *QLoggingCategory::defaultCategory())
        : AkApplicationBase(std::make_unique<T>(argc, argv), loggingCategory)
    {
        init();
    }
};

template<typename T>
class AkUniqueApplicationImpl : public AkApplicationBase
{
public:
    AkUniqueApplicationImpl(int &argc, char **argv, const QString &serviceName, const QLoggingCategory &loggingCategory = *QLoggingCategory::defaultCategory())
        : AkApplicationBase(std::make_unique<T>(argc, argv), loggingCategory)
    {
        registerUniqueServiceOrTerminate(serviceName, loggingCategory);
        init();
    }

private:
    void registerUniqueServiceOrTerminate(const QString &serviceName, const QLoggingCategory &log)
    {
        auto bus = QDBusConnection::sessionBus();
        if (!bus.isConnected()) {
            qCCritical(log, "Session bus not found. Is DBus running?");
            exit(1);
        }

        if (!bus.registerService(serviceName)) {
            // We couldn't register. Most likely, it's already running.
            const QString lastError = bus.lastError().message();
            if (lastError.isEmpty()) {
                qCInfo(log, "Service %s already registered, terminating now.", qUtf8Printable(serviceName));
                exit(0); // already running, so it's OK. Terminate now.
            } else {
                qCCritical(log, "Unable to register service as %s due to an error: %s", qUtf8Printable(serviceName), qUtf8Printable(lastError));
                exit(1); // :(
            }
        }
    }
};

/**
 * Returns the contents of @p name environment variable if it is defined,
 * or @p defaultValue otherwise.
 */
QString akGetEnv(const char *name, const QString &defaultValue = QString());

typedef AkApplicationImpl<QCoreApplication> AkCoreApplication;
typedef AkApplicationImpl<QApplication> AkApplication;
typedef AkApplicationImpl<QGuiApplication> AkGuiApplication;
typedef AkUniqueApplicationImpl<QGuiApplication> AkUniqueGuiApplication;

#endif
