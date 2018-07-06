/*
    Copyright (c) 2008 Volker Krause <vkrause@kde.org>

    Inspired by kdelibs/kdecore/io/kdebug.h
    Copyright (C) 1997 Matthias Kalle Dalheimer (kalle@kde.org)
                  2002 Holger Freyther (freyther@kde.org)

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

#include "akdebug.h"

#include <private/standarddirs_p.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QLoggingCategory>

#include <KCrash/KCrash>

#include <cassert>

class FileDebugStream : public QIODevice
{
    Q_OBJECT
public:
    FileDebugStream()
        : mType(QtCriticalMsg)
    {
        open(WriteOnly);
    }

    bool isSequential() const override
    {
        return true;
    }
    qint64 readData(char *, qint64) override {
        return 0;
    }
    qint64 readLineData(char *, qint64) override {
        return 0;
    }

    qint64 writeData(const char *data, qint64 len) override {
        if (!mFileName.isEmpty())
        {
            QFile outputFile(mFileName);
            outputFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Unbuffered);
            outputFile.write(data, len);
            outputFile.putChar('\n');
            outputFile.close();
        }

        return len;
    }

    void setFileName(const QString &fileName)
    {
        mFileName = fileName;
    }

    void setType(QtMsgType type)
    {
        mType = type;
    }
private:
    QString mFileName;
    QtMsgType mType;
};

class DebugPrivate
{
public:
    DebugPrivate()
        : origHandler(nullptr)
    {
    }

    ~DebugPrivate()
    {
        qInstallMessageHandler(origHandler);
        file.close();
    }

    static QString errorLogFileName(const QString &name)
    {
        return Akonadi::StandardDirs::saveDir("data")
                + QDir::separator()
                + name
                + QLatin1String(".error");
    }

    QString errorLogFileName() const
    {
        return errorLogFileName(name);
    }

    void log(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        QMutexLocker locker(&mutex);
#ifdef QT_NO_DEBUG_OUTPUT
        if (type == QtDebugMsg) {
            return;
        }
#endif
        const QByteArray buf = msg.toUtf8();
        file.write(buf.constData(), buf.size());
        file.write("\n");
        file.flush();

        if (origHandler) {
            origHandler(type, context, msg);
        }
    }

    void setName(const QString &appName)
    {
        name = appName;

        if (file.isOpen()) {
            file.close();
        }
        QFileInfo finfo(errorLogFileName());
        if (!finfo.absoluteDir().exists()) {
            QDir().mkpath(finfo.absolutePath());
        }
        file.setFileName(errorLogFileName());
        file.open(QIODevice::WriteOnly | QIODevice::Unbuffered);
    }

    void setOrigHandler(QtMessageHandler origHandler_)
    {
        origHandler = origHandler_;
    }

    void setOrigCategoryFilter(QLoggingCategory::CategoryFilter origFilter_)
    {
        origFilter = origFilter_;
    }

    QMutex mutex;
    QFile file;
    QString name;
    QtMessageHandler origHandler;
    QLoggingCategory::CategoryFilter origFilter;
    QByteArray loggingCategory;
};

Q_GLOBAL_STATIC(DebugPrivate, sInstance)

void akMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    switch (type) {
    case QtDebugMsg:
    case QtInfoMsg:
    case QtWarningMsg:
    case QtCriticalMsg:
        sInstance()->log(type, context, msg);
        break;
    case QtFatalMsg:
        sInstance()->log(QtInfoMsg, context, msg);
        abort();
    }
}

void akCategoryFilter(QLoggingCategory *category)
{
    if ((qstrcmp(category->categoryName(), sInstance()->loggingCategory) == 0) ||
            (qstrcmp(category->categoryName(), "org.kde.pim.akonadiprivate") == 0)) {
        category->setEnabled(QtDebugMsg, true);
        category->setEnabled(QtInfoMsg, true);
        category->setEnabled(QtWarningMsg, true);
        category->setEnabled(QtCriticalMsg, true);
        category->setEnabled(QtFatalMsg, true);
    } else if (sInstance()->origFilter) {
        sInstance()->origFilter(category);
    }
}

void akInit(const QString &appName)
{
    KCrash::initialize();

    const QString name = QFileInfo(appName).fileName();
    const auto errorLogFile = DebugPrivate::errorLogFileName(name);
    QFileInfo infoOld(errorLogFile + QLatin1String(".old"));
    if (infoOld.exists()) {
        QFile fileOld(infoOld.absoluteFilePath());
        const bool success = fileOld.remove();
        if (!success) {
            qFatal("Cannot remove old log file '%s': %s",
                qUtf8Printable(fileOld.fileName()),
                qUtf8Printable(fileOld.errorString()));
        }
    }

    QFileInfo info(errorLogFile);
    if (info.exists()) {
        QFile file(info.absoluteFilePath());
        const QString oldName = errorLogFile + QLatin1String(".old");
        const bool success = file.copy(oldName);
        if (!success) {
            qFatal("Cannot rename log file '%s' to '%s': %s",
                   qUtf8Printable(file.fileName()),
                   qUtf8Printable(oldName),
                   qUtf8Printable(file.errorString()));
        }
    }

    QtMessageHandler origHandler = qInstallMessageHandler(akMessageHandler);
    sInstance()->setName(name);
    sInstance()->setOrigHandler(origHandler);
}

void akMakeVerbose(const QByteArray &category)
{
    sInstance()->loggingCategory = category;
    QLoggingCategory::CategoryFilter oldFilter = QLoggingCategory::installFilter(akCategoryFilter);
    sInstance()->setOrigCategoryFilter(oldFilter);
}

#include "akdebug.moc"

