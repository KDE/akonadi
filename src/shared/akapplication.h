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
    AkApplicationBase(int &argc, char **argv, const QLoggingCategory &loggingCategory);
    void init();
    QScopedPointer<QCoreApplication> mApp;

private Q_SLOTS:
    void pollSessionBus() const;

private:
    int mArgc;
    char **mArgv;
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
        : AkApplicationBase(argc, argv, loggingCategory)
    {
        mApp.reset(new T(argc, argv));
        init();
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

#endif
